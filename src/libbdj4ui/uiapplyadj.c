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
#include "tagdef.h"
#include "ui.h"
#include "callback.h"
#include "uiapplyadj.h"

enum {
  UIAA_CB_DIALOG,
  UIAA_CB_MAX,
};

typedef struct uiaa {
  UIWidget        *parentwin;
  UIWidget        *statusMsg;
  nlist_t         *options;
  UIWidget        aaDialog;
  callback_t      *callbacks [UIAA_CB_MAX];
  callback_t      *responsecb;
  song_t          *song;
  bool            isactive : 1;
} uiaa_t;

static void   uiaaCreateDialog (uiaa_t *uiaa);
static void   uiaaInitDisplay (uiaa_t *uiaa);
static bool   uiaaResponseHandler (void *udata, long responseid);

uiaa_t *
uiaaInit (UIWidget *windowp, nlist_t *opts)
{
  uiaa_t  *uiaa;

  uiaa = mdmalloc (sizeof (uiaa_t));
  uiutilsUIWidgetInit (&uiaa->aaDialog);
  uiaa->parentwin = windowp;
  uiaa->statusMsg = NULL;
  uiaa->options = opts;
  uiaa->song = NULL;
  for (int i = 0; i < UIAA_CB_MAX; ++i) {
    uiaa->callbacks [i] = NULL;
  }
  uiaa->responsecb = NULL;
  uiaa->isactive = false;

  return uiaa;
}

void
uiaaFree (uiaa_t *uiaa)
{
  if (uiaa != NULL) {
    for (int i = 0; i < UIAA_CB_MAX; ++i) {
      callbackFree (uiaa->callbacks [i]);
    }
    if (uiaa->song != NULL) {
      songFree (uiaa->song);
    }
    uiDialogDestroy (&uiaa->aaDialog);
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
uiaaDialog (uiaa_t *uiaa)
{
  int         x, y;

  if (uiaa == NULL) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uiaaDialog");
  uiaaCreateDialog (uiaa);
  uiaaInitDisplay (uiaa);
  uiWidgetShowAll (&uiaa->aaDialog);
  uiaa->isactive = true;

  x = nlistGetNum (uiaa->options, APPLY_ADJ_POSITION_X);
  y = nlistGetNum (uiaa->options, APPLY_ADJ_POSITION_Y);
  uiWindowMove (&uiaa->aaDialog, x, y, -1);
  logProcEnd (LOG_PROC, "uiaaDialog", "");
  return UICB_CONT;
}

/* internal routines */

static void
uiaaCreateDialog (uiaa_t *uiaa)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      sg;  // labels
  UIWidget      sgA; // title, artist

  logProcBegin (LOG_PROC, "uiaaCreateDialog");

  if (uiaa == NULL) {
    return;
  }

  if (uiutilsUIWidgetSet (&uiaa->aaDialog)) {
    return;
  }

  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgA);

  uiaa->callbacks [UIAA_CB_DIALOG] = callbackInitLong (
      uiaaResponseHandler, uiaa);
  uiCreateDialog (&uiaa->aaDialog, uiaa->parentwin,
      uiaa->callbacks [UIAA_CB_DIALOG],
      /* CONTEXT: apply adjustment dialog: title for the dialog */
      _("Apply Adjustments"),
      /* CONTEXT: apply adjustment dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      /* CONTEXT: apply adjustment dialog: apply adjustments */
      _("Apply Adjustments"),
      RESPONSE_APPLY,
      NULL
      );

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, 10);
  uiWidgetSetMarginTop (&vbox, 20);
  uiWidgetExpandHoriz (&vbox);
  uiWidgetExpandVert (&vbox);
  uiDialogPackInDialog (&uiaa->aaDialog, &vbox);

  /* trim silence */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateCheckButton (&uiwidget,
      /* CONTEXT: apply adjustments: trim silence checkbox */
      _("Trim Silence"), 0);

  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  /* normalize audio */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateCheckButton (&uiwidget,
      /* CONTEXT: apply adjustments: normalize volume checkbox */
      _("Normalize Volume"), 0);

  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  /* normalize audio */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateCheckButton (&uiwidget,
      /* CONTEXT: apply adjustments: apply adjustments checkbox */
      _("Adjust Speed, Song Start and End"), 0);

  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

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

  uiWindowGetPosition (&uiaa->aaDialog, &x, &y, &ws);
  nlistSetNum (uiaa->options, APPLY_ADJ_POSITION_X, x);
  nlistSetNum (uiaa->options, APPLY_ADJ_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: apply adjust: del window");
      uiutilsUIWidgetInit (&uiaa->aaDialog);
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: apply adjust: close window");
      uiWidgetHide (&uiaa->aaDialog);
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: apply adjust: apply");
      uiWidgetHide (&uiaa->aaDialog);
      if (uiaa->responsecb != NULL) {
        callbackHandler (uiaa->responsecb);
      }
      break;
    }
  }

  uiaa->isactive = false;
  return UICB_CONT;
}

