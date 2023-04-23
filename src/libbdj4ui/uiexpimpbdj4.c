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

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "ui.h"
#include "callback.h"
#include "uiexpimpbdj4.h"

enum {
  UIEXPIMPBDJ4_CB_DIALOG,
  UIEXPIMPBDJ4_CB_TARGET,
  UIEXPIMPBDJ4_CB_MAX,
};

typedef struct uiexpimpbdj4 {
  uiwcont_t       *parentwin;
  uiwcont_t       *statusMsg [UIEXPIMPBDJ4_MAX];
  nlist_t         *options;
  uiwcont_t       *dialog [UIEXPIMPBDJ4_MAX];
  uientry_t       *target [UIEXPIMPBDJ4_MAX];
  uibutton_t      *targetButton [UIEXPIMPBDJ4_MAX];
  int             currdialog;
  callback_t      *callbacks [UIEXPIMPBDJ4_CB_MAX];
  callback_t      *responsecb;
  bool            isactive : 1;
} uiexpimpbdj4_t;

/* export/import bdj4 */
static void   uiexpimpbdj4CreateDialog (uiexpimpbdj4_t *uiexpimpbdj4);
static bool   uiexpimpbdj4TargetDialog (void *udata);
static void   uiexpimpbdj4InitDisplay (uiexpimpbdj4_t *uiexpimpbdj4);
static bool   uiexpimpbdj4ResponseHandler (void *udata, long responseid);

uiexpimpbdj4_t *
uiexpimpbdj4Init (uiwcont_t *windowp, nlist_t *opts)
{
  uiexpimpbdj4_t  *uiexpimpbdj4;

  uiexpimpbdj4 = mdmalloc (sizeof (uiexpimpbdj4_t));
  for (int i = 0; i < UIEXPIMPBDJ4_MAX; ++i) {
    uiexpimpbdj4->dialog [i] = NULL;
    uiexpimpbdj4->target [i] = uiEntryInit (50, MAXPATHLEN);
    uiexpimpbdj4->statusMsg [i] = NULL;
    uiexpimpbdj4->targetButton [i] = NULL;
  }
  uiexpimpbdj4->parentwin = windowp;
  uiexpimpbdj4->options = opts;
  for (int i = 0; i < UIEXPIMPBDJ4_CB_MAX; ++i) {
    uiexpimpbdj4->callbacks [i] = NULL;
  }
  uiexpimpbdj4->responsecb = NULL;
  uiexpimpbdj4->isactive = false;

  return uiexpimpbdj4;
}

void
uiexpimpbdj4Free (uiexpimpbdj4_t *uiexpimpbdj4)
{
  if (uiexpimpbdj4 != NULL) {
    for (int i = 0; i < UIEXPIMPBDJ4_CB_MAX; ++i) {
      callbackFree (uiexpimpbdj4->callbacks [i]);
    }
    for (int i = 0; i < UIEXPIMPBDJ4_MAX; ++i) {
      uiwcontFree (uiexpimpbdj4->dialog [i]);
      uiEntryFree (uiexpimpbdj4->target [i]);
      uiButtonFree (uiexpimpbdj4->targetButton [i]);
    }
    mdfree (uiexpimpbdj4);
  }
}

void
uiexpimpbdj4SetResponseCallback (uiexpimpbdj4_t *uiexpimpbdj4, callback_t *uicb)
{
  if (uiexpimpbdj4 == NULL) {
    return;
  }
  uiexpimpbdj4->responsecb = uicb;
}

bool
uiexpimpbdj4Dialog (uiexpimpbdj4_t *uiexpimpbdj4, int expimptype)
{
  int         x, y;

  if (uiexpimpbdj4 == NULL) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uiexpimpbdj4Dialog");
  uiexpimpbdj4->currdialog = expimptype;
  uiexpimpbdj4CreateDialog (uiexpimpbdj4);
  uiDialogShow (uiexpimpbdj4->dialog [expimptype]);
  uiexpimpbdj4->isactive = true;

  x = nlistGetNum (uiexpimpbdj4->options, EXP_IMP_BDJ4_POSITION_X);
  y = nlistGetNum (uiexpimpbdj4->options, EXP_IMP_BDJ4_POSITION_Y);
  uiWindowMove (uiexpimpbdj4->dialog [expimptype], x, y, -1);
  logProcEnd (LOG_PROC, "uiexpimpbdj4Dialog", "");
  return UICB_CONT;
}

/* internal routines */

static void
uiexpimpbdj4CreateDialog (uiexpimpbdj4_t *uiexpimpbdj4)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp;
  uibutton_t    *uibutton;
  uiwcont_t     *szgrp;  // labels
  const char    *buttontext;
  int           currdialog;

  logProcBegin (LOG_PROC, "uiexpimpbdj4CreateDialog");

  if (uiexpimpbdj4 == NULL) {
    return;
  }

  currdialog = uiexpimpbdj4->currdialog;
  buttontext = "";
  if (currdialog == UIEXPIMPBDJ4_EXPORT) {
    /* CONTEXT: export/import bdj4 dialog: export */
    buttontext = _("Export");
  }
  if (currdialog == UIEXPIMPBDJ4_IMPORT) {
    /* CONTEXT: export/import bdj4 dialog: import */
    buttontext = _("Import");
  }

  if (uiexpimpbdj4->dialog [currdialog] != NULL) {
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();

  uiexpimpbdj4->callbacks [UIEXPIMPBDJ4_CB_DIALOG] = callbackInitLong (
      uiexpimpbdj4ResponseHandler, uiexpimpbdj4);
  uiexpimpbdj4->dialog [currdialog] = uiCreateDialog (uiexpimpbdj4->parentwin,
      uiexpimpbdj4->callbacks [UIEXPIMPBDJ4_CB_DIALOG],
      /* CONTEXT: export/import bdj4 dialog: title for the dialog */
      _("Select Target Folder"),
      /* CONTEXT: export/import bdj4 request external dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      buttontext,
      RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 10);
  uiWidgetSetMarginTop (vbox, 20);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiDialogPackInDialog (uiexpimpbdj4->dialog [currdialog], vbox);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: request external: enter the audio file location */
      _("Folder"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiEntryCreate (uiexpimpbdj4->target [currdialog]);
  uiEntrySetValue (uiexpimpbdj4->target [currdialog], "");
  uiwidgetp = uiEntryGetWidgetContainer (uiexpimpbdj4->target [currdialog]);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);

  uiexpimpbdj4->callbacks [UIEXPIMPBDJ4_CB_TARGET] = callbackInit (
      uiexpimpbdj4TargetDialog, uiexpimpbdj4, NULL);
  uibutton = uiCreateButton (
      uiexpimpbdj4->callbacks [UIEXPIMPBDJ4_CB_TARGET],
      "", NULL);
  uiexpimpbdj4->targetButton [currdialog] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: export/import bdj4: playlist: select the playlist */
  uiwidgetp = uiCreateColonLabel (_("Playlist"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

//  uiwidgetp = uiComboboxCreate (uiexpimpbdj4->cfplsel,
//      uiexpimpbdj4->wcont [UIEXPIMPBDJ4_W_CFPL_DIALOG], "",
//      uiexpimpbdj4->callbacks [UIEXPIMPBDJ4_CB_CFPL_PLAYLIST_SEL], uiexpimpbdj4);
//  uiexpimpbdj4CFPLCreatePlaylistList (uiexpimpbdj4);
//  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (hbox);
  uiwcontFree (vbox);
  uiwcontFree (szgrp);

  logProcEnd (LOG_PROC, "uiexpimpbdj4CreateDialog", "");
}

static bool
uiexpimpbdj4TargetDialog (void *udata)
{
  uiexpimpbdj4_t  *uiexpimpbdj4 = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;

  if (uiexpimpbdj4 == NULL) {
    return UICB_STOP;
  }

  selectdata = uiDialogCreateSelect (uiexpimpbdj4->parentwin,
      /* CONTEXT: export/import bdj4 folder selection dialog: window title */
      _("Select Folder"), "/", NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    /* the validation process will be called */
    uiEntrySetValue (uiexpimpbdj4->target [uiexpimpbdj4->currdialog], fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    mdfree (fn);   // allocated by gtk
  }
  mdfree (selectdata);

  return UICB_CONT;
}


static void
uiexpimpbdj4InitDisplay (uiexpimpbdj4_t *uiexpimpbdj4)
{
  if (uiexpimpbdj4 == NULL) {
    return;
  }

}

static bool
uiexpimpbdj4ResponseHandler (void *udata, long responseid)
{
  uiexpimpbdj4_t  *uiexpimpbdj4 = udata;
  int             x, y, ws;
  int             currdialog;

  currdialog = uiexpimpbdj4->currdialog;
  uiWindowGetPosition (uiexpimpbdj4->dialog [currdialog], &x, &y, &ws);
  nlistSetNum (uiexpimpbdj4->options, EXP_IMP_BDJ4_POSITION_X, x);
  nlistSetNum (uiexpimpbdj4->options, EXP_IMP_BDJ4_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: del window");
      uiwcontFree (uiexpimpbdj4->dialog [currdialog]);
      uiexpimpbdj4->dialog [currdialog] = NULL;
      uiEntryFree (uiexpimpbdj4->target [currdialog]);
      uiexpimpbdj4->target [currdialog] = NULL;
      uiButtonFree (uiexpimpbdj4->targetButton [currdialog]);
      uiexpimpbdj4->targetButton [currdialog] = NULL;
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: close window");
      uiWidgetHide (uiexpimpbdj4->dialog [currdialog]);
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: expimpbdj4: apply");
      uiWidgetHide (uiexpimpbdj4->dialog [currdialog]);
      if (uiexpimpbdj4->responsecb != NULL) {
        callbackHandler (uiexpimpbdj4->responsecb);
      }
      break;
    }
  }

  uiexpimpbdj4->isactive = false;
  return UICB_CONT;
}
