/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "audiotag.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "ui.h"
#include "callback.h"
#include "uicopytags.h"
#include "uiselectfile.h"

enum {
  UICT_CB_DIALOG,
  UICT_CB_SOURCE_SEL,
  UICT_CB_TARGET_SEL,
  UICT_CB_MAX,
};

typedef struct uict {
  uiwcont_t       *parentwin;
  nlist_t         *options;
  uiwcont_t       *ctDialog;
  uiwcont_t       *statusMsg;
  uientry_t       *source;
  uibutton_t      *sourcesel;
  uisfcb_t        sourcesfcb;
  uientry_t       *target;
  uibutton_t      *targetsel;
  uisfcb_t        targetsfcb;
  callback_t      *callbacks [UICT_CB_MAX];
  int             state;
  bool            isactive : 1;
} uict_t;

static void   uicopytagsCreateDialog (uict_t *uict);
static bool   uicopytagsResponseHandler (void *udata, long responseid);

uict_t *
uicopytagsInit (uiwcont_t *windowp, nlist_t *opts)
{
  uict_t  *uict;

  uict = mdmalloc (sizeof (uict_t));
  uict->ctDialog = NULL;
  uict->parentwin = windowp;
  uict->options = opts;
  uict->statusMsg = NULL;
  uict->source = uiEntryInit (50, 200);
  uict->sourcesel = NULL;
  uict->target = uiEntryInit (50, 200);
  uict->targetsel = NULL;
  for (int i = 0; i < UICT_CB_MAX; ++i) {
    uict->callbacks [i] = NULL;
  }
  uict->isactive = false;
  uict->state = BDJ4_STATE_OFF;

  selectInitCallback (&uict->sourcesfcb);
  uict->sourcesfcb.title = NULL;
  uict->sourcesfcb.entry = uict->source;
  uict->sourcesfcb.window = uict->parentwin;
  selectInitCallback (&uict->targetsfcb);
  uict->targetsfcb.title = NULL;
  uict->targetsfcb.entry = uict->target;
  uict->targetsfcb.window = uict->parentwin;

  uict->callbacks [UICT_CB_DIALOG] = callbackInitLong (
      uicopytagsResponseHandler, uict);

  return uict;
}

void
uicopytagsFree (uict_t *uict)
{
  if (uict != NULL) {
    uiEntryFree (uict->source);
    uiEntryFree (uict->target);
    uiButtonFree (uict->sourcesel);
    uiButtonFree (uict->targetsel);
    uiwcontFree (uict->ctDialog);
    uiwcontFree (uict->statusMsg);
    for (int i = 0; i < UICT_CB_MAX; ++i) {
      callbackFree (uict->callbacks [i]);
    }
    mdfree (uict);
  }
}

bool
uicopytagsDialog (uict_t *uict)
{
  int         x, y;

  if (uict == NULL) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uicopytagsDialog");
  uict->state = BDJ4_STATE_WAIT;
  uicopytagsCreateDialog (uict);
  uiDialogShow (uict->ctDialog);
  uict->isactive = true;

  x = nlistGetNum (uict->options, COPY_TAGS_POSITION_X);
  y = nlistGetNum (uict->options, COPY_TAGS_POSITION_Y);
  if (x >= 0 && y >= 0) {
    uiWindowMove (uict->ctDialog, x, y, -1);
  }

  logProcEnd (LOG_PROC, "uicopytagsDialog", "");
  return UICB_CONT;
}

void
uicopytagsProcess (uict_t *uict)
{
  if (uict == NULL) {
    return;
  }

  uiEntryValidate (uict->source, false);
  uiEntryValidate (uict->target, false);
}

int
uicopytagsState (uict_t *uict)
{
  return uict->state;
}

const char *
uicopytagsGetFilename (uict_t *uict)
{
  return uiEntryGetValue (uict->target);
}

void
uicopytagsReset (uict_t *uict)
{
  uiWidgetHide (uict->ctDialog);
  uict->state = BDJ4_STATE_OFF;
}

/* internal routines */

static void
uicopytagsCreateDialog (uict_t *uict)
{
  uiwcont_t    *vbox;
  uiwcont_t    *hbox;
  uiwcont_t    *uiwidgetp;

  logProcBegin (LOG_PROC, "uicopytagsCreateDialog");

  if (uict == NULL) {
    return;
  }

  if (uict->ctDialog != NULL) {
    return;
  }

  uict->ctDialog = uiCreateDialog (uict->parentwin,
      uict->callbacks [UICT_CB_DIALOG],
      /* CONTEXT: copy tags dialog: title for the dialog */
      _("Copy Audio Tags"),
      /* CONTEXT: copy tags dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      NULL);
  uiDialogAddButtons (uict->ctDialog,
      /* CONTEXT: copy tags dialog: copy audio tags button */
      _("Copy Audio Tags"),
      RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiDialogPackInDialog (uict->ctDialog, vbox);

  /* status message */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  uict->statusMsg = uiwidgetp;

  uiwcontFree (hbox);

  /* source */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: song editor: copy tags: source file */
  uiwidgetp = uiCreateColonLabel (_("Source"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiEntryCreate (uict->source);
  uiwidgetp = uiEntryGetWidgetContainer (uict->source);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiEntrySetValidate (uict->source,
      uiEntryValidateFile, NULL, UIENTRY_DELAYED);

  uict->callbacks [UICT_CB_SOURCE_SEL] = callbackInit (
      selectAllFileCallback, &uict->sourcesfcb, NULL);
  uict->sourcesel = uiCreateButton (
      uict->callbacks [UICT_CB_SOURCE_SEL], "", NULL);
  uiButtonSetImageIcon (uict->sourcesel, "folder");
  uiwidgetp = uiButtonGetWidgetContainer (uict->sourcesel);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);

  /* target */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: song editor: copy tags: target file */
  uiwidgetp = uiCreateColonLabel (_("Target"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiEntryCreate (uict->target);
  uiwidgetp = uiEntryGetWidgetContainer (uict->target);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiEntrySetValidate (uict->target,
      uiEntryValidateFile, NULL, UIENTRY_DELAYED);

  uict->callbacks [UICT_CB_TARGET_SEL] = callbackInit (
      selectAudioFileCallback, &uict->targetsfcb, NULL);
  uict->targetsel = uiCreateButton (
      uict->callbacks [UICT_CB_TARGET_SEL], "", NULL);
  uiButtonSetImageIcon (uict->targetsel, "folder");
  uiwidgetp = uiButtonGetWidgetContainer (uict->targetsel);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);
  uiwcontFree (vbox);

  logProcEnd (LOG_PROC, "uicopytagsCreateDialog", "");
}

static bool
uicopytagsResponseHandler (void *udata, long responseid)
{
  uict_t      *uict = udata;
  int         x, y, ws;

  uiWindowGetPosition (uict->ctDialog, &x, &y, &ws);
  nlistSetNum (uict->options, COPY_TAGS_POSITION_X, x);
  nlistSetNum (uict->options, COPY_TAGS_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: copy tags: del window");
      /* dialog was destroyed */
      uiwcontFree (uict->ctDialog);
      uict->ctDialog = NULL;
      uict->state = BDJ4_STATE_OFF;
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: copy tags: close window");
      uiWidgetHide (uict->ctDialog);
      uict->state = BDJ4_STATE_OFF;
      break;
    }
    case RESPONSE_APPLY: {
      const char  *infn;
      const char  *outfn;
      void        *sdata;

      logMsg (LOG_DBG, LOG_ACTIONS, "= action: copy tags");
      uiLabelSetText (uict->statusMsg, "");
      infn = uiEntryGetValue (uict->source);
      outfn = uiEntryGetValue (uict->target);
      if (! fileopFileExists (infn) ||
          ! fileopFileExists (outfn)) {
        /* CONTEXT: copy audio tags: one of the files does not exist */
        uiLabelSetText (uict->statusMsg, _("Invalid Selections"));
        break;
      }
      sdata = audiotagSaveTags (infn);
      audiotagRestoreTags (outfn, sdata);
      audiotagFreeSavedTags (outfn, sdata);
      /* CONTEXT: copy audio tags: copy of audio tags completed */
      uiLabelSetText (uict->statusMsg, _("Complete"));
      uict->state = BDJ4_STATE_FINISH;
      break;
    }
  }

  uict->isactive = false;
  return UICB_CONT;
}

