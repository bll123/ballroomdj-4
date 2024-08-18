/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "dance.h"
#include "istring.h"
#include "manageui.h"
#include "mdebug.h"
#include "playlist.h"
#include "tagdef.h"
#include "tmutil.h"
#include "ui.h"
#include "uivirtlist.h"
#include "validate.h"

enum {
  MPLDNC_COL_DANCE_SELECT,
  MPLDNC_COL_DANCE,
  MPLDNC_COL_COUNT,
  MPLDNC_COL_MAXPLAYTIME,
  MPLDNC_COL_LOWMPM,
  MPLDNC_COL_HIGHMPM,
  MPLDNC_COL_DANCE_KEY,
  MPLDNC_COL_MAX,
};

enum {
  MPLDNC_CB_UNSEL,
  MPLDNC_CB_CHANGED,
  MPLDNC_CB_MAX,
};

typedef struct mpldance {
  manageinfo_t      *minfo;
  uivirtlist_t      *uivl;
  uiwcont_t         *uihideunsel;
  callback_t        *callbacks [MPLDNC_CB_MAX];
  playlist_t        *playlist;
  nlist_t           *currlist;
  int               currcount;
  bool              changed : 1;
  bool              hideunselected : 1;
  bool              inchange : 1;
} mpldance_t;

static void manageplDanceSetColumnVisibility (mpldance_t *mpldnc, int pltype);
static bool manageplDanceChanged (void *udata);
static bool manageplDanceHideUnselectedCB (void *udata);
static int  manageplDanceMPMDisplay (ilistidx_t dkey, int mpm);
static int  manageplDanceMPMConvert (ilistidx_t dkey, int mpm);
static void manageplDanceColumnHeading (char *tbuff, size_t sz, const char *disp, const char *bpmstr);
static void manageplDanceFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);
static void manageplDanceRebuildCurrList (mpldance_t *mpldnc);
static void manageplDancePopulate (mpldance_t *mpldnc);

mpldance_t *
manageplDanceAlloc (manageinfo_t *minfo)
{
  mpldance_t     *mpldnc;

  mpldnc = mdmalloc (sizeof (mpldance_t));
  mpldnc->minfo = minfo;
  mpldnc->uivl = NULL;
  mpldnc->playlist = NULL;
  mpldnc->currlist = NULL;
  mpldnc->currcount = 0;
  mpldnc->changed = false;
  mpldnc->uihideunsel = NULL;
  mpldnc->hideunselected = false;
  mpldnc->inchange = false;
  for (int i = 0; i < MPLDNC_CB_MAX; ++i) {
    mpldnc->callbacks [i] = NULL;
  }

  return mpldnc;
}

void
manageplDanceFree (mpldance_t *mpldnc)
{
  if (mpldnc == NULL) {
    return;
  }

  uiwcontFree (mpldnc->uihideunsel);
  nlistFree (mpldnc->currlist);
  uivlFree (mpldnc->uivl);
  for (int i = 0; i < MPLDNC_CB_MAX; ++i) {
    callbackFree (mpldnc->callbacks [i]);
  }
  mdfree (mpldnc);
}

void
manageplDanceBuildUI (mpldance_t *mpldnc, uiwcont_t *vboxp)
{
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;
  const char  *bpmstr;
  char        tbuff [100];

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vboxp, hbox);
  uiWidgetSetAllMargins (hbox, 2);
  uiWidgetAlignHorizEnd (hbox);

  /* CONTEXT: playlist management: hide unselected dances */
  uiwidgetp = uiCreateCheckButton (_("Hide Unselected"), 0);
  mpldnc->callbacks [MPLDNC_CB_UNSEL] = callbackInit (
      manageplDanceHideUnselectedCB, mpldnc, NULL);
  uiToggleButtonSetCallback (uiwidgetp, mpldnc->callbacks [MPLDNC_CB_UNSEL]);
  uiBoxPackStart (hbox, uiwidgetp);
  mpldnc->uihideunsel = uiwidgetp;

  mpldnc->uivl = uivlCreate ("mpl-dance", mpldnc->minfo->window, vboxp,
      10, 300, VL_FLAGS_NONE);
  uivlSetNumColumns (mpldnc->uivl, MPLDNC_COL_MAX);
  uivlMakeColumn (mpldnc->uivl, "sel", MPLDNC_COL_DANCE_SELECT, VL_TYPE_CHECKBOX);
  uivlMakeColumn (mpldnc->uivl, "dnc", MPLDNC_COL_DANCE, VL_TYPE_LABEL);
  uivlMakeColumnSpinboxNum (mpldnc->uivl, "count", MPLDNC_COL_COUNT, 0.0, 100.0, 1.0, 5.0);
  uivlMakeColumnSpinboxTime (mpldnc->uivl, "mpt", MPLDNC_COL_MAXPLAYTIME, SB_TIME_BASIC, NULL);
  uivlMakeColumnSpinboxNum (mpldnc->uivl, "lowmpm", MPLDNC_COL_LOWMPM, 0.0, 500.0, 1.0, 5.0);
  uivlMakeColumnSpinboxNum (mpldnc->uivl, "himpm", MPLDNC_COL_HIGHMPM, 0.0, 500.0, 1.0, 5.0);
  uivlMakeColumn (mpldnc->uivl, "dkey", MPLDNC_COL_DANCE_KEY, VL_TYPE_INTERNAL_NUMERIC);

  bpmstr = tagdefs [TAG_BPM].displayname;

  /* this helps to prevent the column width from bouncing around */
  uivlSetColumnMinWidth (mpldnc->uivl, MPLDNC_COL_DANCE, 20);

  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_DANCE_SELECT, "");
  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_DANCE, tagdefs [TAG_DANCE].displayname);
  /* CONTEXT: playlist management: count column header */
  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_COUNT, _("Count"));
  /* CONTEXT: playlist management: max play time column header (keep short) */
  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_MAXPLAYTIME, _("Maximum\nPlay Time"));
  /* CONTEXT: playlist management: low bpm/mpm column header */
  manageplDanceColumnHeading (tbuff, sizeof (tbuff), _("Low %s"), bpmstr);
  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_LOWMPM, tbuff);
  /* CONTEXT: playlist management: high bpm/mpm column header */
  manageplDanceColumnHeading (tbuff, sizeof (tbuff), _("High %s"), bpmstr);
  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_HIGHMPM, tbuff);

  mpldnc->callbacks [MPLDNC_CB_CHANGED] = callbackInit (
      manageplDanceChanged, mpldnc, NULL);

  uivlSetCheckboxChangeCallback (mpldnc->uivl, MPLDNC_COL_DANCE_SELECT,
      mpldnc->callbacks [MPLDNC_CB_CHANGED]);
  uivlSetSpinboxChangeCallback (mpldnc->uivl, MPLDNC_COL_COUNT,
      mpldnc->callbacks [MPLDNC_CB_CHANGED]);
  uivlSetSpinboxTimeChangeCallback (mpldnc->uivl, MPLDNC_COL_MAXPLAYTIME,
      mpldnc->callbacks [MPLDNC_CB_CHANGED]);
  uivlSetSpinboxChangeCallback (mpldnc->uivl, MPLDNC_COL_LOWMPM,
      mpldnc->callbacks [MPLDNC_CB_CHANGED]);
  uivlSetSpinboxChangeCallback (mpldnc->uivl, MPLDNC_COL_HIGHMPM,
      mpldnc->callbacks [MPLDNC_CB_CHANGED]);

  uivlSetRowFillCallback (mpldnc->uivl, manageplDanceFillRow, mpldnc);

  uiwcontFree (hbox);

  manageplDanceRebuildCurrList (mpldnc);
  uivlDisplay (mpldnc->uivl);
}

void
manageplDanceSetPlaylist (mpldance_t *mpldnc, playlist_t *pl)
{
  int     pltype;
  int     hideunselstate = UI_TOGGLE_BUTTON_OFF;
  int     widgetstate = UIWIDGET_DISABLE;

  mpldnc->playlist = pl;
  mpldnc->inchange = true;
  pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);

  if (pltype == PLTYPE_SONGLIST) {
    /* a song list displays all the dances, as other dances */
    /* could be added to the song list via requests, etc. */
    widgetstate = UIWIDGET_DISABLE;
  }
  if (pltype == PLTYPE_SEQUENCE) {
    /* a sequence has no need to display the non-selected dances */
    hideunselstate = UI_TOGGLE_BUTTON_ON;
    widgetstate = UIWIDGET_DISABLE;
  }
  if (pltype == PLTYPE_AUTO) {
    widgetstate = UIWIDGET_ENABLE;
  }

  if (hideunselstate == UI_TOGGLE_BUTTON_OFF) {
    mpldnc->hideunselected = false;
  } else {
    mpldnc->hideunselected = true;
  }

  uiWidgetSetState (mpldnc->uihideunsel, widgetstate);
  uiToggleButtonSetValue (mpldnc->uihideunsel, hideunselstate);

  manageplDanceRebuildCurrList (mpldnc);

  mpldnc->inchange = false;

  manageplDancePopulate (mpldnc);
}

bool
manageplDanceIsChanged (mpldance_t *mpldnc)
{
  return mpldnc->changed;
}

/* internal routines */

static void
manageplDanceSetColumnVisibility (mpldance_t *mpldnc, int pltype)
{
  switch (pltype) {
    case PLTYPE_SONGLIST: {
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_DANCE_SELECT, VL_COL_HIDE);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_COUNT, VL_COL_HIDE);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_LOWMPM, VL_COL_HIDE);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_HIGHMPM, VL_COL_HIDE);
      break;
    }
    case PLTYPE_SEQUENCE: {
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_DANCE_SELECT, VL_COL_HIDE);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_COUNT, VL_COL_HIDE);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_LOWMPM, VL_COL_SHOW);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_HIGHMPM, VL_COL_SHOW);
      break;
    }
    case PLTYPE_AUTO: {
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_DANCE_SELECT, VL_COL_SHOW);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_COUNT, VL_COL_SHOW);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_LOWMPM, VL_COL_SHOW);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_HIGHMPM, VL_COL_SHOW);
      break;
    }
  }
}

static bool
manageplDanceChanged (void *udata)
{
  mpldance_t      *mpldnc = udata;
  playlist_t      *pl;
  ilistidx_t      dkey;
  int32_t         rownum;
  int32_t         val;

  if (mpldnc->inchange) {
    return UICB_CONT;
  }

  rownum = uivlGetCurrSelection (mpldnc->uivl);
  dkey = uivlGetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_DANCE_KEY);
  pl = mpldnc->playlist;

  val = uivlGetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_DANCE_SELECT);
  playlistSetDanceNum (pl, dkey, PLDANCE_SELECTED, val);
  val = uivlGetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_COUNT);
  playlistSetDanceNum (pl, dkey, PLDANCE_COUNT, val);
  val = uivlGetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_MAXPLAYTIME);
  playlistSetDanceNum (pl, dkey, PLDANCE_MAXPLAYTIME, val);
  val = uivlGetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_LOWMPM);
  val = manageplDanceMPMConvert (dkey, val);
  playlistSetDanceNum (pl, dkey, PLDANCE_MPM_LOW, val);
  val = uivlGetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_HIGHMPM);
  val = manageplDanceMPMConvert (dkey, val);
  playlistSetDanceNum (pl, dkey, PLDANCE_MPM_HIGH, val);

  mpldnc->changed = true;

  return UICB_CONT;
}

static bool
manageplDanceHideUnselectedCB (void *udata)
{
  mpldance_t      *mpldnc = udata;
  bool            tchg;

  if (mpldnc->inchange) {
    return UICB_STOP;
  }

  tchg = mpldnc->changed;
  mpldnc->hideunselected = ! mpldnc->hideunselected;
  manageplDanceRebuildCurrList (mpldnc);
  manageplDancePopulate (mpldnc);
  mpldnc->changed = tchg;
  return UICB_CONT;
}

static int
manageplDanceMPMDisplay (ilistidx_t dkey, int mpm)
{
  if (mpm < 0) {
    mpm = 0;
  }

  mpm = danceConvertMPMtoBPM (dkey, mpm);
  return mpm;
}

static int
manageplDanceMPMConvert (ilistidx_t dkey, int mpm)
{
  if (mpm < 0) {
    mpm = 0;
  }

  mpm = danceConvertBPMtoMPM (dkey, mpm, DANCE_NO_FORCE);
  return mpm;
}

/* modify long column headers (for low bpm/high bpm) */
static void
manageplDanceColumnHeading (char *tbuff, size_t sz, const char *disp,
    const char *bpmstr)
{
  snprintf (tbuff, sz, disp, bpmstr);
  if (istrlen (tbuff) > 8) {
    char    *p;

    p = strstr (tbuff, " ");
    if (p != NULL) {
      *p = '\n';
    }
  }
}

static void
manageplDanceFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  mpldance_t  *mpldnc = udata;
  dance_t     *dances;
  playlist_t  *pl;
  ilistidx_t  dkey;
  const char  *dancedisp;
  int32_t     val;

  dkey = nlistGetNum (mpldnc->currlist, rownum);
  if (dkey < 0) {
    return;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  pl = mpldnc->playlist;

  mpldnc->inchange = true;

  dancedisp = danceGetStr (dances, dkey, DANCE_DANCE);
  uivlSetRowColumnStr (mpldnc->uivl, rownum, MPLDNC_COL_DANCE, dancedisp);
  uivlSetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_DANCE_KEY, dkey);

  val = playlistGetDanceNum (pl, dkey, PLDANCE_COUNT);
  if (val == LIST_VALUE_INVALID) { val = 0; }
  uivlSetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_COUNT, val);

  val = playlistGetDanceNum (pl, dkey, PLDANCE_SELECTED);
  if (val == LIST_VALUE_INVALID) { val = 0; }
  uivlSetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_DANCE_SELECT, val);

  val = playlistGetDanceNum (pl, dkey, PLDANCE_MAXPLAYTIME);
  if (val == LIST_VALUE_INVALID) { val = 0; }
  uivlSetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_MAXPLAYTIME, val);

  val = playlistGetDanceNum (pl, dkey, PLDANCE_MPM_LOW);
  val = manageplDanceMPMDisplay (dkey, val);
  uivlSetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_LOWMPM, val);

  val = playlistGetDanceNum (pl, dkey, PLDANCE_MPM_HIGH);
  val = manageplDanceMPMDisplay (dkey, val);
  uivlSetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_HIGHMPM, val);

  mpldnc->inchange = false;
}

/* create a row-number to dance mapping */
static void
manageplDanceRebuildCurrList (mpldance_t *mpldnc)
{
  dance_t       *dances;
  slist_t       *dancelist;
  slistidx_t    iteridx;
  ilistidx_t    dcount;
  ilistidx_t    ccount;
  ilistidx_t    dkey;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancelist = danceGetDanceList (dances);
  dcount = danceGetCount (dances);
  ccount = 0;

  nlistFree (mpldnc->currlist);
  mpldnc->currlist = nlistAlloc ("mpldnc-curr", LIST_UNORDERED, NULL);
  nlistSetSize (mpldnc->currlist, dcount);
  slistStartIterator (dancelist, &iteridx);
  while ((dkey = slistIterateValueNum (dancelist, &iteridx)) >= 0) {
    if (mpldnc->hideunselected && mpldnc->playlist != NULL) {
      int     sel;

      sel = playlistGetDanceNum (mpldnc->playlist, dkey, PLDANCE_SELECTED);
      if (! sel) {
        continue;
      }
    }

    nlistSetNum (mpldnc->currlist, ccount, dkey);
    ++ccount;
  }
  nlistSort (mpldnc->currlist);

  uivlSetNumRows (mpldnc->uivl, ccount);
}

static void
manageplDancePopulate (mpldance_t *mpldnc)
{
  playlist_t    *pl;
  int           pltype;

  pl = mpldnc->playlist;
  pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);

  manageplDanceSetColumnVisibility (mpldnc, pltype);

  uivlPopulate (mpldnc->uivl);
  mpldnc->changed = false;
}


