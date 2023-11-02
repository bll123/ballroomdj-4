/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
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
#include "bdjstring.h"
#include "callback.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "sysvars.h"
#include "tagdef.h"
#include "ui.h"
#include "uiquickedit.h"
#include "uirating.h"
#include "uiutils.h"
#include "validate.h"

enum {
  UIQE_CB_DIALOG,
  UIQE_CB_SPEED_SCALE,
  UIQE_CB_VOLUME_SCALE,
  UIQE_CB_MAX,
};

enum {
  UIQE_W_DIALOG,
  UIQE_W_TITLE_DISP,
  UIQE_W_SZGRP_LABEL,
  UIQE_W_SZGRP_SCALE,
  UIQE_W_SZGRP_SCALE_DISP,
  UIQE_W_MAX,
};

enum {
  UIQE_SCALE_SPD,
  UIQE_SCALE_VOL,
  UIQE_SCALE_MAX,
};

typedef struct {
  int           tagkey;
  uiwcont_t     *label;
  uiwcont_t     *scale;
  uiwcont_t     *scaledisp;
  callback_t    *scalecb;
} uiqescale_t;

typedef struct uiqe {
  uiwcont_t         *parentwin;
  musicdb_t         *musicdb;
  nlist_t           *options;
  uiwcont_t         *wcont [UIQE_W_MAX];
  uirating_t        *uirating;
  callback_t        *responsecb;
  callback_t        *callbacks [UIQE_CB_MAX];
  uiqescale_t       scaledata [UIQE_SCALE_MAX];
  dbidx_t           dbidx;
  bool              isactive : 1;
} uiqe_t;

/* quick edit */
static void   uiqeCreateDialog (uiqe_t *uiqe);
static void   uiqeInitDisplay (uiqe_t *uiqe);
static bool   uiqeResponseHandler (void *udata, long responseid);
static void   uiqeAddScale (uiqe_t *uiqe, uiwcont_t *hbox, int scidx);
static bool   uiqeScaleDisplayCallback (void *udata, double value);

uiqe_t *
uiqeInit (uiwcont_t *windowp, musicdb_t *musicdb, nlist_t *opts)
{
  uiqe_t  *uiqe;

  uiqe = mdmalloc (sizeof (uiqe_t));
  for (int i = 0; i < UIQE_W_MAX; ++i) {
    uiqe->wcont [i] = NULL;
  }
  for (int i = 0; i < UIQE_SCALE_MAX; ++i) {
    uiqe->scaledata [i].tagkey = -1;
    uiqe->scaledata [i].label = NULL;
    uiqe->scaledata [i].scale = NULL;
    uiqe->scaledata [i].scaledisp = NULL;
  }
  uiqe->responsecb = NULL;
  uiqe->parentwin = windowp;
  uiqe->musicdb = musicdb;
  uiqe->options = opts;
  for (int i = 0; i < UIQE_CB_MAX; ++i) {
    uiqe->callbacks [i] = NULL;
  }
  uiqe->isactive = false;
  uiqe->uirating = NULL;

  uiqe->callbacks [UIQE_CB_DIALOG] = callbackInitLong (
      uiqeResponseHandler, uiqe);

  return uiqe;
}

void
uiqeFree (uiqe_t *uiqe)
{
  if (uiqe == NULL) {
    return;
  }

  for (int i = 0; i < UIQE_CB_MAX; ++i) {
    callbackFree (uiqe->callbacks [i]);
    uiqe->callbacks [i] = NULL;
  }
  for (int i = 0; i < UIQE_W_MAX; ++i) {
    uiwcontFree (uiqe->wcont [i]);
    uiqe->wcont [i] = NULL;
  }
  for (int i = 0; i < UIQE_SCALE_MAX; ++i) {
    uiwcontFree (uiqe->scaledata [i].label);
    uiwcontFree (uiqe->scaledata [i].scale);
    uiwcontFree (uiqe->scaledata [i].scaledisp);
    uiqe->scaledata [i].label = NULL;
    uiqe->scaledata [i].scale = NULL;
    uiqe->scaledata [i].scaledisp = NULL;
  }
  uiratingFree (uiqe->uirating);
  uiqe->uirating = NULL;
  mdfree (uiqe);
}

void
uiqeSetResponseCallback (uiqe_t *uiqe, callback_t *uicb)
{
  if (uiqe == NULL) {
    return;
  }
  uiqe->responsecb = uicb;
}

bool
uiqeDialog (uiqe_t *uiqe, dbidx_t dbidx)
{
  int         x, y;

  if (uiqe == NULL) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uiqeDialog");
  uiqe->dbidx = dbidx;
  uiqeCreateDialog (uiqe);
  uiqeInitDisplay (uiqe);
  uiDialogShow (uiqe->wcont [UIQE_W_DIALOG]);
  uiqe->isactive = true;

  x = nlistGetNum (uiqe->options, QE_POSITION_X);
  y = nlistGetNum (uiqe->options, QE_POSITION_Y);
  uiWindowMove (uiqe->wcont [UIQE_W_DIALOG], x, y, -1);
  logProcEnd (LOG_PROC, "uiqeDialog", "");
  return UICB_CONT;
}

/* internal routines */

static void
uiqeCreateDialog (uiqe_t *uiqe)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp = NULL;

  logProcBegin (LOG_PROC, "uiqeCreateDialog");

  if (uiqe == NULL) {
    return;
  }

  if (uiqe->wcont [UIQE_W_DIALOG] != NULL) {
    return;
  }


  uiqe->wcont [UIQE_W_SZGRP_LABEL] = uiCreateSizeGroupHoriz ();
  uiqe->wcont [UIQE_W_SZGRP_SCALE] = uiCreateSizeGroupHoriz ();
  uiqe->wcont [UIQE_W_SZGRP_SCALE_DISP] = uiCreateSizeGroupHoriz ();

  uiqe->wcont [UIQE_W_DIALOG] =
      uiCreateDialog (uiqe->parentwin,
      uiqe->callbacks [UIQE_CB_DIALOG],
      /* CONTEXT: quick edit: title of dialog */
      _("Quick Edit"),
      /* CONTEXT: quick edit dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      /* CONTEXT: quick edit dialog: save button */
      _("Save"),
      RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiDialogPackInDialog (uiqe->wcont [UIQE_W_DIALOG], vbox);

  /* begin line: title display */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("title");
  uiBoxPackStart (hbox, uiwidgetp);
  uiqe->wcont [UIQE_W_TITLE_DISP] = uiwidgetp;

  /* begin line: speed scale */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);
  uiqeAddScale (uiqe, hbox, UIQE_SCALE_SPD);
  uiwcontFree (hbox);

  /* begin line: volume scale */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);
  uiqeAddScale (uiqe, hbox, UIQE_SCALE_VOL);
  uiwcontFree (hbox);

  /* begin line: volume scale */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCERATING].displayname);
  uiSizeGroupAdd (uiqe->wcont [UIQE_W_SZGRP_LABEL], uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);

  uiqe->uirating = uiratingSpinboxCreate (hbox, UIRATING_NORM);
  uiwcontFree (hbox);

  uiwcontFree (vbox);

  logProcEnd (LOG_PROC, "uiqeCreateDialog", "");
}

static void
uiqeInitDisplay (uiqe_t *uiqe)
{
  if (uiqe == NULL) {
    return;
  }
}

static bool
uiqeResponseHandler (void *udata, long responseid)
{
  uiqe_t  *uiqe = udata;
  int             x, y, ws;

  uiWindowGetPosition (uiqe->wcont [UIQE_W_DIALOG], &x, &y, &ws);
  nlistSetNum (uiqe->options, QE_POSITION_X, x);
  nlistSetNum (uiqe->options, QE_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: quick edit: del window");
      uiwcontFree (uiqe->wcont [UIQE_W_DIALOG]);
      uiqe->wcont [UIQE_W_DIALOG] = NULL;
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: quick edit: close window");
      uiWidgetHide (uiqe->wcont [UIQE_W_DIALOG]);
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: quick edit: save");
      if (uiqe->responsecb != NULL) {
        callbackHandler (uiqe->responsecb);
      }
      uiWidgetHide (uiqe->wcont [UIQE_W_DIALOG]);
      break;
    }
  }

  uiqe->isactive = false;
  return UICB_CONT;
}

static void
uiqeAddScale (uiqe_t *uiqe, uiwcont_t *hbox, int scidx)
{
  uiwcont_t       *uiwidgetp;
  double          lower, upper;
  double          inca, incb;
  int             digits;
  int             tagkey;

  logProcBegin (LOG_PROC, "uiqeAddScale");
  if (scidx == UIQE_SCALE_SPD) {
    tagkey = TAG_SPEEDADJUSTMENT;
    lower = SPD_LOWER;
    upper = SPD_UPPER;
    digits = SPD_DIGITS;
    inca = SPD_INCA;
    incb = SPD_INCB;
  }
  if (scidx == UIQE_SCALE_VOL) {
    tagkey = TAG_VOLUMEADJUSTPERC;
    lower = VOL_LOWER;
    upper = VOL_UPPER;
    digits = VOL_DIGITS;
    inca = VOL_INCA;
    incb = VOL_INCB;
  }
  uiqe->scaledata [scidx].tagkey = tagkey;

  uiwidgetp = uiCreateColonLabel (tagdefs [tagkey].displayname);
  uiSizeGroupAdd (uiqe->wcont [UIQE_W_SZGRP_LABEL], uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiqe->scaledata [scidx].label = uiwidgetp;

  uiwidgetp = uiCreateScale (lower, upper, inca, incb, 0.0, digits);
  uiqe->scaledata [scidx].scale = uiwidgetp;
  uiqe->scaledata [scidx].scalecb = callbackInitDouble (
      uiqeScaleDisplayCallback, &uiqe->scaledata [scidx]);
  uiScaleSetCallback (uiwidgetp, uiqe->scaledata [scidx].scalecb);

  uiSizeGroupAdd (uiqe->wcont [UIQE_W_SZGRP_SCALE], uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwidgetp = uiCreateLabel ("100%");
  uiWidgetSetMarginStart (uiwidgetp, 2);
  uiLabelAlignEnd (uiwidgetp);
  uiSizeGroupAdd (uiqe->wcont [UIQE_W_SZGRP_SCALE_DISP], uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiqe->scaledata [scidx].scaledisp = uiwidgetp;
  logProcEnd (LOG_PROC, "uiqeAddScale", "");
}

/* also sets the changed flag */
static bool
uiqeScaleDisplayCallback (void *udata, double value)
{
  uiqescale_t *scaledata = udata;
  char        tbuff [40];
  int         digits;

  logProcBegin (LOG_PROC, "uiqeScaleDisplayCallback");
  digits = uiScaleGetDigits (scaledata->scale);
  value = uiScaleEnforceMax (scaledata->scale, value);
  snprintf (tbuff, sizeof (tbuff), "%4.*f%%", digits, value);
  uiLabelSetText (scaledata->scaledisp, tbuff);
  logProcEnd (LOG_PROC, "uiqeScaleDisplayCallback", "");
  return UICB_CONT;
}

