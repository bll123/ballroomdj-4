/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
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
#include "bdjopt.h"
#include "bdjstring.h"
#include "callback.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "song.h"
#include "sysvars.h"
#include "tagdef.h"
#include "ui.h"
#include "uifavorite.h"
#include "uilevel.h"
#include "uiquickedit.h"
#include "uirating.h"
#include "uiutils.h"
#include "validate.h"

enum {
  UIQE_CB_DIALOG,
  UIQE_CB_SPEED_SCALE,
  UIQE_CB_VOLADJ_SCALE,
  UIQE_CB_MAX,
};

enum {
  UIQE_W_DIALOG,
  UIQE_W_ARTIST_DISP,
  UIQE_W_TITLE_DISP,
  UIQE_W_SZGRP_LABEL,
  UIQE_W_SZGRP_SCALE,
  UIQE_W_SZGRP_SCALE_DISP,
  UIQE_W_MAX,
};

enum {
  UIQE_SCALE_SPD,
  UIQE_SCALE_VOLADJ,
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
  uilevel_t         *uilevel;
  uirating_t        *uirating;
  uifavorite_t      *uifavorite;
  callback_t        *responsecb;
  callback_t        *callbacks [UIQE_CB_MAX];
  uiqescale_t       scaledata [UIQE_SCALE_MAX];
  song_t            *song;
  dbidx_t           dbidx;
  uiqesave_t        savedata;
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
    uiqe->scaledata [i].scalecb = NULL;
  }
  uiqe->responsecb = NULL;
  uiqe->parentwin = windowp;
  uiqe->musicdb = musicdb;
  uiqe->options = opts;
  for (int i = 0; i < UIQE_CB_MAX; ++i) {
    uiqe->callbacks [i] = NULL;
  }
  uiqe->isactive = false;
  uiqe->uilevel = NULL;
  uiqe->uirating = NULL;
  uiqe->uifavorite = NULL;
  uiqe->savedata.dbidx = -1;
  uiqe->savedata.speed = 100.0;
  uiqe->savedata.voladj = 0.0;
  uiqe->savedata.rating = 0;
  uiqe->savedata.level = 0;

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
    callbackFree (uiqe->scaledata [i].scalecb);
    uiqe->scaledata [i].label = NULL;
    uiqe->scaledata [i].scale = NULL;
    uiqe->scaledata [i].scaledisp = NULL;
  }
  uilevelFree (uiqe->uilevel);
  uiratingFree (uiqe->uirating);
  uifavoriteFree (uiqe->uifavorite);
  uiqe->uilevel = NULL;
  uiqe->uirating = NULL;
  uiqe->uifavorite = NULL;
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
uiqeDialog (uiqe_t *uiqe, dbidx_t dbidx, double speed, double vol, int basevol)
{
  int         x, y;
  const char  *artist;
  const char  *title;
  int         levelidx;
  int         ratingidx;
  int         fav;
  double      voladj;
  double      svoladj;
  int         dfltvol;

  if (uiqe == NULL) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uiqeDialog");

  uiqe->dbidx = dbidx;

  if (dbidx < 0 || dbidx >= dbCount (uiqe->musicdb)) {
    logProcEnd (LOG_PROC, "uiqeDialog", "bad-dbidx");
    return UICB_STOP;
  }

  uiqeCreateDialog (uiqe);
  uiqeInitDisplay (uiqe);
  uiDialogShow (uiqe->wcont [UIQE_W_DIALOG]);
  uiqe->isactive = true;

  uiqe->song = dbGetByIdx (uiqe->musicdb, dbidx);
  if (uiqe->song == NULL) {
    logProcEnd (LOG_PROC, "uiqeDialog", "null-song");
    return UICB_STOP;
  }

  artist = songGetStr (uiqe->song, TAG_ARTIST);
  uiLabelSetText (uiqe->wcont [UIQE_W_ARTIST_DISP], artist);

  title = songGetStr (uiqe->song, TAG_TITLE);
  uiLabelSetText (uiqe->wcont [UIQE_W_TITLE_DISP], title);

  ratingidx = songGetNum (uiqe->song, TAG_DANCERATING);
  uiratingSetValue (uiqe->uirating, ratingidx);

  levelidx = songGetNum (uiqe->song, TAG_DANCELEVEL);
  uilevelSetValue (uiqe->uilevel, levelidx);

  fav = songGetNum (uiqe->song, TAG_FAVORITE);
  uifavoriteSetValue (uiqe->uifavorite, fav);

  if (speed == LIST_DOUBLE_INVALID) {
    speed = (double) songGetNum (uiqe->song, TAG_SPEEDADJUSTMENT);
  }
  if (speed <= 0.0) {
    speed = 100.0;
  }
  uiScaleSetValue (uiqe->scaledata [UIQE_SCALE_SPD].scale, speed);
  uiqeScaleDisplayCallback (&uiqe->scaledata [UIQE_SCALE_SPD], speed);

  if (vol == LIST_DOUBLE_INVALID) {
    voladj = songGetDouble (uiqe->song, TAG_VOLUMEADJUSTPERC);
  } else {
    /* the volume adjustment is the song's volume-adjust-perc plus */
    /* any changes to the current volume from the default volume */
    dfltvol = basevol;
    if (dfltvol <= 0) {
      dfltvol = bdjoptGetNum (OPT_P_DEFAULTVOLUME);
    }
    voladj = vol - (double) dfltvol;
    svoladj = songGetDouble (uiqe->song, TAG_VOLUMEADJUSTPERC);
    if (svoladj == LIST_DOUBLE_INVALID) {
      svoladj = 0.0;
    }
    voladj += svoladj;
  }
  if (voladj == LIST_DOUBLE_INVALID) {
    voladj = 0.0;
  }
  uiScaleSetValue (uiqe->scaledata [UIQE_SCALE_VOLADJ].scale, voladj);
  uiqeScaleDisplayCallback (&uiqe->scaledata [UIQE_SCALE_VOLADJ], voladj);

  x = nlistGetNum (uiqe->options, QE_POSITION_X);
  y = nlistGetNum (uiqe->options, QE_POSITION_Y);
  uiWindowMove (uiqe->wcont [UIQE_W_DIALOG], x, y, -1);
  logProcEnd (LOG_PROC, "uiqeDialog", "");
  return UICB_CONT;
}

const uiqesave_t *
uiqeGetResponseData (uiqe_t *uiqe)
{
  if (uiqe == NULL) {
    return NULL;
  }
  return &uiqe->savedata;
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

  uiwidgetp = uiCreateLabel ("");
  uiLabelEllipsizeOn (uiwidgetp);
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uiBoxPackStart (hbox, uiwidgetp);
  uiqe->wcont [UIQE_W_ARTIST_DISP] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiLabelEllipsizeOn (uiwidgetp);
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uiWidgetSetMarginStart (uiwidgetp, 10);
  uiBoxPackStart (hbox, uiwidgetp);
  uiqe->wcont [UIQE_W_TITLE_DISP] = uiwidgetp;

  /* begin line: speed scale */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);
  uiqeAddScale (uiqe, hbox, UIQE_SCALE_SPD);
  uiwcontFree (hbox);

  /* begin line: vol-adj scale */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);
  uiqeAddScale (uiqe, hbox, UIQE_SCALE_VOLADJ);
  uiwcontFree (hbox);

  /* begin line: rating */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCERATING].displayname);
  uiSizeGroupAdd (uiqe->wcont [UIQE_W_SZGRP_LABEL], uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);

  uiqe->uirating = uiratingSpinboxCreate (hbox, UIRATING_NORM);
  uiwcontFree (hbox);

  /* begin line: level */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCELEVEL].displayname);
  uiSizeGroupAdd (uiqe->wcont [UIQE_W_SZGRP_LABEL], uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);

  uiqe->uilevel = uilevelSpinboxCreate (hbox, UIRATING_NORM);
  uiwcontFree (hbox);

  /* begin line: favorite */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_FAVORITE].displayname);
  uiSizeGroupAdd (uiqe->wcont [UIQE_W_SZGRP_LABEL], uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);

  uiqe->uifavorite = uifavoriteSpinboxCreate (hbox);
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
  int     x, y, ws;

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
      uiqe->savedata.dbidx = uiqe->dbidx;
      uiqe->savedata.speed =
          uiScaleGetValue (uiqe->scaledata [UIQE_SCALE_SPD].scale);
      uiqe->savedata.voladj =
          uiScaleGetValue (uiqe->scaledata [UIQE_SCALE_VOLADJ].scale);
      uiqe->savedata.rating = uiratingGetValue (uiqe->uirating);
      uiqe->savedata.level = uilevelGetValue (uiqe->uilevel);
      uiqe->savedata.favorite = uifavoriteGetValue (uiqe->uifavorite);

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
  if (scidx == UIQE_SCALE_VOLADJ) {
    tagkey = TAG_VOLUMEADJUSTPERC;
    lower = VOL_ADJ_LOWER;
    upper = VOL_ADJ_UPPER;
    digits = VOL_ADJ_DIGITS;
    inca = VOL_ADJ_INCA;
    incb = VOL_ADJ_INCB;
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

