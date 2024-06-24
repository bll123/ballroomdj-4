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
  MPLDNC_COL_SB_PAD,
  MPLDNC_COL_DANCE_IDX,
  MPLDNC_COL_EDITABLE,
  MPLDNC_COL_ADJUST,
  MPLDNC_COL_DIGITS,
  MPLDNC_COL_MAX,
};

enum {
  MPLDNC_CB_UNSEL,
  MPLDNC_CB_CHANGED,
  MPLDNC_CB_MAX,
};

typedef struct mpldance {
  uiwcont_t         *uitree;
  uivirtlist_t      *uivl;
  uiwcont_t         *errorMsg;
  uiwcont_t         *uihideunsel;
  callback_t        *callbacks [MPLDNC_CB_MAX];
  playlist_t        *playlist;
  int               currcount;
  bool              changed : 1;
  bool              hideunselected : 1;
  bool              inprepop : 1;
} mpldance_t;

static void managePLDanceSetColumnVisibility (mpldance_t *mpldnc, int pltype);
static bool managePLDanceChanged (void *udata, int32_t col);
static void managePLDanceCreate (mpldance_t *mpldnc);
static bool managePLDanceHideUnselectedCallback (void *udata);
static int  managePLDanceBPMDisplay (ilistidx_t dkey, int bpm);
static void managePLDanceColumnHeading (char *tbuff, size_t sz, const char *disp, const char *bpmstr);
static void managePLDanceFillRow (void *udata, uivirtlist_t *vl, int32_t rownum);

mpldance_t *
managePLDanceAlloc (uiwcont_t *errorMsg)
{
  mpldance_t     *mpldnc;

  mpldnc = mdmalloc (sizeof (mpldance_t));
  mpldnc->uitree = NULL;
  mpldnc->uivl = NULL;
  mpldnc->errorMsg = errorMsg;
  mpldnc->playlist = NULL;
  mpldnc->currcount = 0;
  mpldnc->changed = false;
  mpldnc->uihideunsel = NULL;
  mpldnc->hideunselected = false;
  mpldnc->inprepop = false;
  for (int i = 0; i < MPLDNC_CB_MAX; ++i) {
    mpldnc->callbacks [i] = NULL;
  }

  return mpldnc;
}

void
managePLDanceFree (mpldance_t *mpldnc)
{
  if (mpldnc == NULL) {
    return;
  }

  uiwcontFree (mpldnc->uihideunsel);
  uiwcontFree (mpldnc->uitree);
  uivlFree (mpldnc->uivl);
  for (int i = 0; i < MPLDNC_CB_MAX; ++i) {
    callbackFree (mpldnc->callbacks [i]);
  }
  mdfree (mpldnc);
}

void
manageBuildUIPlaylistTree (mpldance_t *mpldnc, uiwcont_t *vboxp)
{
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;
//  uiwcont_t   *scwindow;
  const char  *bpmstr;
  char        tbuff [100];

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 2);
  uiWidgetAlignHorizEnd (hbox);
  uiBoxPackStart (vboxp, hbox);

  /* CONTEXT: playlist management: hide unselected dances */
  uiwidgetp = uiCreateCheckButton (_("Hide Unselected"), 0);
  mpldnc->callbacks [MPLDNC_CB_UNSEL] = callbackInit (
      managePLDanceHideUnselectedCallback, mpldnc, NULL);
  uiToggleButtonSetCallback (uiwidgetp, mpldnc->callbacks [MPLDNC_CB_UNSEL]);
  uiBoxPackStart (hbox, uiwidgetp);
  mpldnc->uihideunsel = uiwidgetp;

  mpldnc->uivl = uiCreateVirtList ("mpl-dance", vboxp, 15, VL_SHOW_HEADING, 100);
  uivlSetNumColumns (mpldnc->uivl, MPLDNC_COL_MAX);
  uivlMakeColumn (mpldnc->uivl, MPLDNC_COL_DANCE_SELECT, VL_TYPE_CHECK_BUTTON);
  uivlMakeColumn (mpldnc->uivl, MPLDNC_COL_DANCE, VL_TYPE_LABEL);
  uivlMakeColumnSpinboxNum (mpldnc->uivl, MPLDNC_COL_COUNT, 0.0, 100.0, 1.0, 5.0);
  uivlMakeColumnSpinboxTime (mpldnc->uivl, MPLDNC_COL_MAXPLAYTIME, SB_TIME_BASIC, NULL);
  uivlMakeColumnSpinboxNum (mpldnc->uivl, MPLDNC_COL_LOWMPM, 0.0, 500.0, 1.0, 5.0);
  uivlMakeColumnSpinboxNum (mpldnc->uivl, MPLDNC_COL_HIGHMPM, 0.0, 500.0, 1.0, 5.0);
  uivlMakeColumn (mpldnc->uivl, MPLDNC_COL_DANCE_IDX, VL_TYPE_INTERNAL_NUMERIC);

  bpmstr = tagdefs [TAG_BPM].displayname;

  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_DANCE_SELECT, "");
  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_DANCE, tagdefs [TAG_DANCE].displayname);
  /* CONTEXT: playlist management: count column header */
  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_COUNT, _("Count"));
  /* CONTEXT: playlist management: max play time column header (keep short) */
  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_MAXPLAYTIME, _("Maximum\nPlay Time"));
  /* CONTEXT: playlist management: low bpm/mpm column header */
  managePLDanceColumnHeading (tbuff, sizeof (tbuff), _("Low %s"), bpmstr);
  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_LOWMPM, tbuff);
  /* CONTEXT: playlist management: high bpm/mpm column header */
  managePLDanceColumnHeading (tbuff, sizeof (tbuff), _("High %s"), bpmstr);
  uivlSetColumnHeading (mpldnc->uivl, MPLDNC_COL_HIGHMPM, tbuff);

  uivlSetRowFillCallback (mpldnc->uivl, managePLDanceFillRow, mpldnc);

#if 0
  scwindow = uiCreateScrolledWindow (300);
  uiWidgetExpandVert (scwindow);
  uiBoxPackStartExpand (vboxp, scwindow);

  mpldnc->uitree = uiCreateTreeView ();

  mpldnc->callbacks [MPLDNC_CB_CHANGED] = callbackInitI (
      managePLDanceChanged, mpldnc);
  uiTreeViewSetEditedCallback (mpldnc->uitree,
      mpldnc->callbacks [MPLDNC_CB_CHANGED]);

  uiWidgetExpandVert (mpldnc->uitree);
  uiWindowPackInWindow (scwindow, mpldnc->uitree);

  /* done with the scrolled window */
  uiwcontFree (scwindow);

  uiTreeViewEnableHeaders (mpldnc->uitree);

  uiTreeViewAppendColumn (mpldnc->uitree, TREE_NO_COLUMN,
      TREE_WIDGET_CHECKBOX, TREE_ALIGN_CENTER,
      TREE_COL_DISP_GROW, NULL,
      TREE_COL_TYPE_ACTIVE, MPLDNC_COL_DANCE_SELECT,
      TREE_COL_TYPE_END);

  uiTreeViewAppendColumn (mpldnc->uitree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, tagdefs [TAG_DANCE].displayname,
      TREE_COL_TYPE_TEXT, MPLDNC_COL_DANCE,
      TREE_COL_TYPE_END);

  uiTreeViewAppendColumn (mpldnc->uitree, TREE_NO_COLUMN,
      TREE_WIDGET_SPINBOX, TREE_ALIGN_RIGHT,
      /* CONTEXT: playlist management: count column header */
      TREE_COL_DISP_GROW, _("Count"),
      TREE_COL_TYPE_TEXT, MPLDNC_COL_COUNT,
      TREE_COL_TYPE_EDITABLE, MPLDNC_COL_EDITABLE,
      TREE_COL_TYPE_ADJUSTMENT, MPLDNC_COL_ADJUST,
      TREE_COL_TYPE_DIGITS, MPLDNC_COL_DIGITS,
      TREE_COL_TYPE_END);

  uiTreeViewAppendColumn (mpldnc->uitree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_RIGHT,
      /* CONTEXT: playlist management: max play time column header (keep short) */
      TREE_COL_DISP_GROW, _("Maximum\nPlay Time"),
      TREE_COL_TYPE_TEXT, MPLDNC_COL_MAXPLAYTIME,
      TREE_COL_TYPE_EDITABLE, MPLDNC_COL_EDITABLE,
      TREE_COL_TYPE_END);

  bpmstr = tagdefs [TAG_BPM].displayname;

  /* CONTEXT: playlist management: low bpm/mpm column header */
  managePLDanceColumnHeading (tbuff, sizeof (tbuff), _("Low %s"), bpmstr);
  uiTreeViewAppendColumn (mpldnc->uitree, TREE_NO_COLUMN,
      TREE_WIDGET_SPINBOX, TREE_ALIGN_RIGHT,
      TREE_COL_DISP_GROW, tbuff,
      TREE_COL_TYPE_TEXT, MPLDNC_COL_LOWMPM,
      TREE_COL_TYPE_EDITABLE, MPLDNC_COL_EDITABLE,
      TREE_COL_TYPE_ADJUSTMENT, MPLDNC_COL_ADJUST,
      TREE_COL_TYPE_DIGITS, MPLDNC_COL_DIGITS,
      TREE_COL_TYPE_END);

  /* CONTEXT: playlist management: high bpm/mpm column header */
  managePLDanceColumnHeading (tbuff, sizeof (tbuff), _("High %s"), bpmstr);
  uiTreeViewAppendColumn (mpldnc->uitree, TREE_NO_COLUMN,
      TREE_WIDGET_SPINBOX, TREE_ALIGN_RIGHT,
      TREE_COL_DISP_GROW, tbuff,
      TREE_COL_TYPE_TEXT, MPLDNC_COL_HIGHMPM,
      TREE_COL_TYPE_EDITABLE, MPLDNC_COL_EDITABLE,
      TREE_COL_TYPE_ADJUSTMENT, MPLDNC_COL_ADJUST,
      TREE_COL_TYPE_DIGITS, MPLDNC_COL_DIGITS,
      TREE_COL_TYPE_END);

  uiTreeViewAppendColumn (mpldnc->uitree, TREE_NO_COLUMN,
      TREE_WIDGET_TEXT, TREE_ALIGN_NORM,
      TREE_COL_DISP_GROW, NULL,
      TREE_COL_TYPE_TEXT, MPLDNC_COL_SB_PAD,
      TREE_COL_TYPE_END);
#endif

  uiwcontFree (hbox);

  managePLDanceCreate (mpldnc);
  uivlDisplay (mpldnc->uivl);
}

void
managePLDancePrePopulate (mpldance_t *mpldnc, playlist_t *pl)
{
  int     pltype;
  int     hideunselstate = UI_TOGGLE_BUTTON_OFF;
  int     widgetstate = UIWIDGET_DISABLE;

  mpldnc->playlist = pl;
  mpldnc->inprepop = true;
  pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);

  if (pltype == PLTYPE_SONGLIST) {
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
  uiWidgetSetState (mpldnc->uihideunsel, widgetstate);
  uiToggleButtonSetState (mpldnc->uihideunsel, hideunselstate);
  mpldnc->inprepop = false;
}

void
managePLDancePopulate (mpldance_t *mpldnc, playlist_t *pl)
{
  dance_t       *dances;
  int           pltype;
  ilistidx_t    iteridx;
  ilistidx_t    dkey;
  int           count;

  uivlPopulate (mpldnc->uivl);

#if 0
  dances = bdjvarsdfGet (BDJVDF_DANCES);

  mpldnc->playlist = pl;
  pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);

  managePLDanceSetColumnVisibility (mpldnc, pltype);

  danceStartIterator (dances, &iteridx);
  count = 0;
  while ((dkey = danceIterate (dances, &iteridx)) >= 0) {
    long  sel, dcount, mpt;
    int   bpmlow, bpmhigh;
    char  mptdisp [40];

    sel = playlistGetDanceNum (pl, dkey, PLDANCE_SELECTED);

    if (mpldnc->hideunselected && pl != NULL) {
      if (! sel) {
        /* don't display this one */
        continue;
      }
    }

    dcount = playlistGetDanceNum (pl, dkey, PLDANCE_COUNT);
    if (dcount < 0) { dcount = 0; }
    mpt = playlistGetDanceNum (pl, dkey, PLDANCE_MAXPLAYTIME);
    if (mpt < 0) { mpt = 0; }
    tmutilToMS (mpt, mptdisp, sizeof (mptdisp));
    bpmlow = playlistGetDanceNum (pl, dkey, PLDANCE_MPM_LOW);
    bpmlow = managePLDanceBPMDisplay (dkey, bpmlow);
    bpmhigh = playlistGetDanceNum (pl, dkey, PLDANCE_MPM_HIGH);
    bpmhigh = managePLDanceBPMDisplay (dkey, bpmhigh);

    uiTreeViewSelectSet (mpldnc->uitree, count);
    uiTreeViewSetValues (mpldnc->uitree,
        MPLDNC_COL_DANCE_SELECT, (treebool_t) sel,
        MPLDNC_COL_MAXPLAYTIME, mptdisp,
        MPLDNC_COL_COUNT, (treenum_t) dcount,
        MPLDNC_COL_LOWMPM, (treenum_t) bpmlow,
        MPLDNC_COL_HIGHMPM, (treenum_t) bpmhigh,
        TREE_VALUE_END);
    ++count;
  }

  uiTreeViewSelectSet (mpldnc->uitree, 0);
  mpldnc->currcount = count;
#endif
  mpldnc->changed = false;
}


bool
managePLDanceIsChanged (mpldance_t *mpldnc)
{
  return mpldnc->changed;
}

void
managePLDanceUpdatePlaylist (mpldance_t *mpldnc)
{
  playlist_t      *pl;
  long            tval;
  char            *tstr = NULL;
  ilistidx_t      dkey;
  int             count;

  if (! mpldnc->changed) {
    return;
  }

  pl = mpldnc->playlist;

  /* hide unselected may be on, and only the displayed dances will */
  /* be updated */

  uiTreeViewSelectSave (mpldnc->uitree);

  for (count = 0; count < mpldnc->currcount; ++count) {
    uiTreeViewSelectSet (mpldnc->uitree, count);

    tval = uiTreeViewGetValue (mpldnc->uitree, MPLDNC_COL_DANCE_IDX);
    dkey = tval;
    tval = uiTreeViewGetValue (mpldnc->uitree, MPLDNC_COL_DANCE_SELECT);
    playlistSetDanceNum (pl, dkey, PLDANCE_SELECTED, tval);

    tval = uiTreeViewGetValue (mpldnc->uitree, MPLDNC_COL_COUNT);
    playlistSetDanceNum (pl, dkey, PLDANCE_COUNT, tval);

    tstr = uiTreeViewGetValueStr (mpldnc->uitree, MPLDNC_COL_MAXPLAYTIME);
    tval = tmutilStrToMS (tstr);
    dataFree (tstr);
    playlistSetDanceNum (pl, dkey, PLDANCE_MAXPLAYTIME, tval);

    tval = uiTreeViewGetValue (mpldnc->uitree, MPLDNC_COL_LOWMPM);
    tval = danceConvertBPMtoMPM (dkey, tval, DANCE_NO_FORCE);
    playlistSetDanceNum (pl, dkey, PLDANCE_MPM_LOW, tval);

    tval = uiTreeViewGetValue (mpldnc->uitree, MPLDNC_COL_HIGHMPM);
    tval = danceConvertBPMtoMPM (dkey, tval, DANCE_NO_FORCE);
    playlistSetDanceNum (pl, dkey, PLDANCE_MPM_HIGH, tval);
  }

  uiTreeViewSelectRestore (mpldnc->uitree);
}

/* internal routines */

static void
managePLDanceSetColumnVisibility (mpldance_t *mpldnc, int pltype)
{
  switch (pltype) {
    case PLTYPE_SONGLIST: {
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_DANCE_SELECT, VL_COL_HIDE);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_COUNT, VL_COL_HIDE);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_LOWMPM, VL_COL_HIDE);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_HIGHMPM, VL_COL_HIDE);
#if 0
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_DANCE_SELECT, TREE_COLUMN_HIDDEN);
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_COUNT, TREE_COLUMN_HIDDEN);
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_LOWMPM, TREE_COLUMN_HIDDEN);
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_HIGHMPM, TREE_COLUMN_HIDDEN);
#endif
      break;
    }
    case PLTYPE_AUTO: {
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_DANCE_SELECT, VL_COL_SHOW);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_COUNT, VL_COL_SHOW);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_LOWMPM, VL_COL_SHOW);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_HIGHMPM, VL_COL_SHOW);
#if 0
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_DANCE_SELECT, TREE_COLUMN_SHOWN);
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_COUNT, TREE_COLUMN_SHOWN);
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_LOWMPM, TREE_COLUMN_SHOWN);
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_HIGHMPM, TREE_COLUMN_SHOWN);
#endif
      break;
    }
    case PLTYPE_SEQUENCE: {
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_DANCE_SELECT, VL_COL_HIDE);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_COUNT, VL_COL_HIDE);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_LOWMPM, VL_COL_SHOW);
      uivlSetColumnDisplay (mpldnc->uivl, MPLDNC_COL_HIGHMPM, VL_COL_SHOW);
#if 0
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_DANCE_SELECT, TREE_COLUMN_HIDDEN);
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_COUNT, TREE_COLUMN_HIDDEN);
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_LOWMPM, TREE_COLUMN_SHOWN);
      uiTreeViewColumnSetVisible (mpldnc->uitree,
          MPLDNC_COL_HIGHMPM, TREE_COLUMN_SHOWN);
#endif
      break;
    }
  }
}

static bool
managePLDanceChanged (void *udata, int32_t col)
{
  mpldance_t      *mpldnc = udata;
  bool            rc = UICB_CONT;

  uiTreeViewSelectCurrent (mpldnc->uitree);

  uiLabelSetText (mpldnc->errorMsg, "");
  mpldnc->changed = true;

  if (col == MPLDNC_COL_MAXPLAYTIME) {
    char        tbuff [200];
    char        *str;
    bool        val;

    str = uiTreeViewGetValueStr (mpldnc->uitree, MPLDNC_COL_MAXPLAYTIME);
    /* CONTEXT: playlist management: validation: max play time */
    val = validate (tbuff, sizeof (tbuff), _("Maximum Play Time"), str, VAL_HMS);
    if (val == false) {
      uiLabelSetText (mpldnc->errorMsg, tbuff);
      mpldnc->changed = false;
      rc = UICB_STOP;
    }
    dataFree (str);
  }

  if (col == MPLDNC_COL_DANCE_SELECT) {
    int         val;
    ilistidx_t  dkey;

    val = uiTreeViewGetValue (mpldnc->uitree, MPLDNC_COL_DANCE_SELECT);
    dkey = uiTreeViewGetValue (mpldnc->uitree, MPLDNC_COL_DANCE_IDX);
    playlistSetDanceNum (mpldnc->playlist, dkey, PLDANCE_SELECTED, val);
  }

  return rc;
}

static void
managePLDanceCreate (mpldance_t *mpldnc)
{
  dance_t       *dances;
  slist_t       *dancelist;
  slistidx_t    iteridx;
  ilistidx_t    key;
  int           count;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancelist = danceGetDanceList (dances);

  count = danceGetCount (dances);
  slistStartIterator (dancelist, &iteridx);
  while ((key = slistIterateValueNum (dancelist, &iteridx)) >= 0) {
    if (mpldnc->hideunselected &&
        mpldnc->playlist != NULL) {
      int     sel;

      sel = playlistGetDanceNum (mpldnc->playlist, key, PLDANCE_SELECTED);
      if (! sel) {
        --count;
      }
    }
  }

fprintf (stderr, "mpl: set-num-rows %d\n", count);
  uivlSetNumRows (mpldnc->uivl, count);

#if 0
  dance_t       *dances;
  slist_t       *dancelist;
  slistidx_t    iteridx;
  ilistidx_t    key;
  uiwcont_t     *uitree;
  uiwcont_t     *adjustment;

  uitree = mpldnc->uitree;

  uiTreeViewCreateValueStore (uitree, MPLDNC_COL_MAX,
      TREE_TYPE_BOOLEAN,  // dance select
      TREE_TYPE_STRING,   // dance
      TREE_TYPE_NUM,      // count
      TREE_TYPE_STRING,   // max play time
      TREE_TYPE_NUM,      // low bpm
      TREE_TYPE_NUM,      // high bpm
      TREE_TYPE_STRING,   // pad
      TREE_TYPE_NUM,      // dance idx
      TREE_TYPE_INT,      // editable
      TREE_TYPE_WIDGET,   // adjust
      TREE_TYPE_INT,      // digits
      TREE_TYPE_END);

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancelist = danceGetDanceList (dances);

  slistStartIterator (dancelist, &iteridx);
  while ((key = slistIterateValueNum (dancelist, &iteridx)) >= 0) {
    const char  *dancedisp;

    dancedisp = danceGetStr (dances, key, DANCE_DANCE);

    if (mpldnc->hideunselected &&
        mpldnc->playlist != NULL) {
      int     sel;

      sel = playlistGetDanceNum (mpldnc->playlist, key, PLDANCE_SELECTED);
      if (! sel) {
        /* don't display this one */
        continue;
      }
    }

    uiTreeViewValueAppend (uitree);
    adjustment = uiCreateAdjustment (0, 0.0, 400.0, 1.0, 5.0, 0.0);
    uiTreeViewSetValues (mpldnc->uitree,
        MPLDNC_COL_DANCE_SELECT, (treebool_t) 0,
        MPLDNC_COL_DANCE, dancedisp,
        MPLDNC_COL_MAXPLAYTIME, "0:00",
        MPLDNC_COL_LOWMPM, (treenum_t) 0,
        MPLDNC_COL_HIGHMPM, (treenum_t) 0,
        MPLDNC_COL_SB_PAD, "  ",
        MPLDNC_COL_DANCE_IDX, (treenum_t) key,
        MPLDNC_COL_EDITABLE, (treeint_t) 1,
        MPLDNC_COL_ADJUST, uiAdjustmentGetAdjustment (adjustment),
        MPLDNC_COL_DIGITS, (treeint_t) 0,
        TREE_VALUE_END);
    uiwcontFree (adjustment);
  }
#endif
}

static bool
managePLDanceHideUnselectedCallback (void *udata)
{
  mpldance_t      *mpldnc = udata;
  bool            tchg;

  tchg = mpldnc->changed;
  if (mpldnc->inprepop) {
    mpldnc->hideunselected =
        uiToggleButtonIsActive (mpldnc->uihideunsel);
  }
  if (! mpldnc->inprepop) {
    mpldnc->hideunselected = ! mpldnc->hideunselected;
  }
  managePLDanceUpdatePlaylist (mpldnc);
  managePLDanceCreate (mpldnc);
  uivlPopulate (mpldnc->uivl);
//  managePLDancePopulate (mpldnc, mpldnc->playlist);
  mpldnc->changed = tchg;
  return UICB_CONT;
}

static int
managePLDanceBPMDisplay (ilistidx_t dkey, int bpm)
{
  if (bpm < 0) {
    bpm = 0;
  }

  bpm = danceConvertMPMtoBPM (dkey, bpm);
  return bpm;
}

/* modify long column headers (for low bpm/high bpm) */
static void
managePLDanceColumnHeading (char *tbuff, size_t sz, const char *disp,
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
managePLDanceFillRow (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  mpldance_t  *mpldnc = udata;
  playlist_t  *pl;
  dance_t     *dances;
  slist_t     *dancelist;
  int         count;
  slistidx_t  iteridx;
  ilistidx_t  dkey;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancelist = danceGetDanceList (dances);
  pl = mpldnc->playlist;

fprintf (stderr, "-- fill rownum: %d\n", rownum);
  /* since some of the dances may be hidden, traverse through the */
  /* list to find the correct dance to display */
  count = 0;
  slistStartIterator (dancelist, &iteridx);
  while ((dkey = slistIterateValueNum (dancelist, &iteridx)) >= 0) {
    int32_t     val;

    val = playlistGetDanceNum (pl, dkey, PLDANCE_SELECTED);
fprintf (stderr, "  dkey: %d/%s count:%d sel:%d\n", dkey, danceGetStr (dances, dkey, DANCE_DANCE), count, val);
    if (mpldnc->hideunselected && pl != NULL) {
      if (! val) {
        continue;
      }
    }

    if (count == rownum) {
      const char  *dancedisp;

      val = playlistGetDanceNum (mpldnc->playlist, dkey, PLDANCE_SELECTED);
      if (val == LIST_VALUE_INVALID) { val = 0; }
      uivlSetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_DANCE_SELECT, val);
      dancedisp = danceGetStr (dances, dkey, DANCE_DANCE);
      uivlSetRowColumnValue (mpldnc->uivl, rownum, MPLDNC_COL_DANCE, dancedisp);
      val = playlistGetDanceNum (mpldnc->playlist, dkey, PLDANCE_MAXPLAYTIME);
      if (val == LIST_VALUE_INVALID) { val = 0; }
      uivlSetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_MAXPLAYTIME, val);
      val = playlistGetDanceNum (mpldnc->playlist, dkey, PLDANCE_MPM_LOW);
      if (val == LIST_VALUE_INVALID) { val = 0; }
      uivlSetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_LOWMPM, val);
      val = playlistGetDanceNum (mpldnc->playlist, dkey, PLDANCE_MPM_HIGH);
      if (val == LIST_VALUE_INVALID) { val = 0; }
      uivlSetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_HIGHMPM, val);
      uivlSetRowColumnNum (mpldnc->uivl, rownum, MPLDNC_COL_DANCE_IDX, dkey);
      break;
    }
    ++count;
  }
}
