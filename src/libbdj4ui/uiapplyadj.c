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
#include <assert.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "slist.h"
#include "song.h"
#include "songutil.h"
#include "tagdef.h"
#include "ui.h"
#include "callback.h"
#include "uiapplyadj.h"

enum {
  UIAA_CB_DIALOG,
  UIAA_CB_MAX,
};

typedef struct uiaa {
  uiwcont_t      *parentwin;
  nlist_t         *options;
  uiwcont_t       *aaDialog;
  uiwcont_t       statusMsg;
  uiwcont_t       *cbTrim;
  uiwcont_t       *cbNorm;
  uiwcont_t       *cbAdjust;
  callback_t      *callbacks [UIAA_CB_MAX];
  callback_t      *responsecb;
  song_t          *song;
  const char      *pleasewaitmsg;
  bool            isactive : 1;
} uiaa_t;

static void   uiaaCreateDialog (uiaa_t *uiaa, int aaflags, bool hasorig);
static void   uiaaInitDisplay (uiaa_t *uiaa);
static bool   uiaaResponseHandler (void *udata, long responseid);

uiaa_t *
uiaaInit (uiwcont_t *windowp, nlist_t *opts)
{
  uiaa_t  *uiaa;

  uiaa = mdmalloc (sizeof (uiaa_t));
  uiaa->aaDialog = NULL;
  uiaa->parentwin = windowp;
  uiaa->options = opts;
  uiaa->song = NULL;
  uiaa->cbTrim = NULL;
  uiaa->cbNorm = NULL;
  uiaa->cbAdjust = NULL;
  for (int i = 0; i < UIAA_CB_MAX; ++i) {
    uiaa->callbacks [i] = NULL;
  }
  uiaa->responsecb = NULL;
  uiaa->isactive = false;
  /* CONTEXT: apply adjustments: please wait... status message */
  uiaa->pleasewaitmsg = _("Please wait\xe2\x80\xa6");

  return uiaa;
}

void
uiaaFree (uiaa_t *uiaa)
{
  if (uiaa != NULL) {
    uiwcontFree (uiaa->cbTrim);
    uiwcontFree (uiaa->cbNorm);
    uiwcontFree (uiaa->cbAdjust);
    for (int i = 0; i < UIAA_CB_MAX; ++i) {
      callbackFree (uiaa->callbacks [i]);
    }
    if (uiaa->song != NULL) {
      songFree (uiaa->song);
    }
    uiDialogDestroy (uiaa->aaDialog);
    uiwcontFree (uiaa->aaDialog);
    mdfree (uiaa);
  }
}

void
uiaaSetResponseCallback (uiaa_t *uiaa, callback_t *uicb)
{
  if (uiaa == NULL) {
    return;
  }
  uiaa->responsecb = uicb;
}

bool
uiaaDialog (uiaa_t *uiaa, int aaflags, bool hasorig)
{
  int         x, y;

  if (uiaa == NULL) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uiaaDialog");
  uiaaCreateDialog (uiaa, aaflags, hasorig);
  uiaaInitDisplay (uiaa);
  uiDialogShow (uiaa->aaDialog);
  uiaa->isactive = true;

  x = nlistGetNum (uiaa->options, APPLY_ADJ_POSITION_X);
  y = nlistGetNum (uiaa->options, APPLY_ADJ_POSITION_Y);
  if (x >= 0 && y >= 0) {
    uiWindowMove (uiaa->aaDialog, x, y, -1);
  }
  logProcEnd (LOG_PROC, "uiaaDialog", "");
  return UICB_CONT;
}

void
uiaaDialogClear (uiaa_t *uiaa)
{
  uiwcontInit (&uiaa->statusMsg);
  uiDialogDestroy (uiaa->aaDialog);
  uiwcontFree (uiaa->aaDialog);
  uiaa->aaDialog = NULL;
}

/* internal routines */

static void
uiaaCreateDialog (uiaa_t *uiaa, int aaflags, bool hasorig)
{
  uiwcont_t    vbox;
  uiwcont_t    hbox;
  uiwcont_t    uiwidget;
  uiwcont_t    *uiwidgetp;

  logProcBegin (LOG_PROC, "uiaaCreateDialog");

  if (uiaa == NULL) {
    return;
  }

  if (uiaa->aaDialog != NULL) {
    return;
  }

  uiaa->callbacks [UIAA_CB_DIALOG] = callbackInitLong (
      uiaaResponseHandler, uiaa);
  uiaa->aaDialog = uiCreateDialog (uiaa->parentwin,
      uiaa->callbacks [UIAA_CB_DIALOG],
      /* CONTEXT: apply adjustment dialog: title for the dialog */
      _("Apply Adjustments"),
      /* CONTEXT: apply adjustment dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      NULL);
  if (hasorig) {
    uiDialogAddButtons (uiaa->aaDialog,
        /* CONTEXT: apply adjustment dialog: restore original file */
        _("Restore Original"),
        RESPONSE_RESET,
        NULL);
  }
  uiDialogAddButtons (uiaa->aaDialog,
      /* CONTEXT: apply adjustment dialog: apply adjustments */
      _("Apply Adjustments"),
      RESPONSE_APPLY,
      NULL
      );

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, 4);
  uiDialogPackInDialog (uiaa->aaDialog, &vbox);

  /* status message */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);
  uiCreateLabel (&uiwidget, "");
  uiWidgetSetClass (&uiwidget, ACCENT_CLASS);
  uiBoxPackEnd (&hbox, &uiwidget);
  uiwcontCopy (&uiaa->statusMsg, &uiwidget);

  /* trim silence */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: apply adjustments: trim silence checkbox */
  uiwidgetp = uiCreateCheckButton (_("Trim Silence"),
      (aaflags & SONG_ADJUST_TRIM) == SONG_ADJUST_TRIM);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiaa->cbTrim = uiwidgetp;

  /* normalize audio */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: apply adjustments: normalize volume checkbox */
  uiwidgetp = uiCreateCheckButton (_("Normalize Volume"),
      (aaflags & SONG_ADJUST_NORM) == SONG_ADJUST_NORM);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiaa->cbNorm = uiwidgetp;

  /* adjust audio */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: apply adjustments: adjust speed/song start/song end checkbox */
  uiwidgetp = uiCreateCheckButton (_("Adjust Speed, Song Start and Song End"),
      (aaflags & SONG_ADJUST_ADJUST) == SONG_ADJUST_ADJUST);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiaa->cbAdjust = uiwidgetp;

  logProcEnd (LOG_PROC, "uiaaCreateDialog", "");
}

static void
uiaaInitDisplay (uiaa_t *uiaa)
{
  if (uiaa == NULL) {
    return;
  }
}

static bool
uiaaResponseHandler (void *udata, long responseid)
{
  uiaa_t  *uiaa = udata;
  int         x, y, ws;

  uiWindowGetPosition (uiaa->aaDialog, &x, &y, &ws);
  nlistSetNum (uiaa->options, APPLY_ADJ_POSITION_X, x);
  nlistSetNum (uiaa->options, APPLY_ADJ_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: apply adjust: del window");
      uiwcontFree (uiaa->aaDialog);
      uiaa->aaDialog = NULL;
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: apply adjust: close window");
      /* dialog should be destroyed, as the buttons are re-created each time */
      uiDialogDestroy (uiaa->aaDialog);
      uiwcontFree (uiaa->aaDialog);
      uiaa->aaDialog = NULL;
      break;
    }
    case RESPONSE_RESET: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: apply adjust: restore orig");
      uiLabelSetText (&uiaa->statusMsg, uiaa->pleasewaitmsg);
      if (uiaa->responsecb != NULL) {
        callbackHandlerLong (uiaa->responsecb, SONG_ADJUST_RESTORE);
      }
      break;
    }
    case RESPONSE_APPLY: {
      long    aaflags = SONG_ADJUST_NONE;

      logMsg (LOG_DBG, LOG_ACTIONS, "= action: apply adjust: apply");
      if (uiToggleButtonIsActive (uiaa->cbTrim)) {
        aaflags |= SONG_ADJUST_TRIM;
      }
      if (uiToggleButtonIsActive (uiaa->cbNorm)) {
        aaflags |= SONG_ADJUST_NORM;
      }
      if (uiToggleButtonIsActive (uiaa->cbAdjust)) {
        aaflags |= SONG_ADJUST_ADJUST;
      }
      uiLabelSetText (&uiaa->statusMsg, uiaa->pleasewaitmsg);
      if (uiaa->responsecb != NULL) {
        callbackHandlerLong (uiaa->responsecb, aaflags);
      }
      break;
    }
  }

  uiaa->isactive = false;
  return UICB_CONT;
}

