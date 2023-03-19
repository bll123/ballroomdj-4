/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "dance.h"
#include "manageui.h"
#include "mdebug.h"
#include "playlist.h"
#include "tagdef.h"
#include "tmutil.h"
#include "ui.h"
#include "validate.h"

enum {
  MPLTREE_COL_DANCE_SELECT,
  MPLTREE_COL_DANCE,
  MPLTREE_COL_COUNT,
  MPLTREE_COL_MAXPLAYTIME,
  MPLTREE_COL_LOWBPM,
  MPLTREE_COL_HIGHBPM,
  MPLTREE_COL_SB_PAD,
  MPLTREE_COL_DANCE_IDX,
  MPLTREE_COL_EDITABLE,
  MPLTREE_COL_ADJUST,
  MPLTREE_COL_DIGITS,
  MPLTREE_COL_MAX,
};

enum {
  MPLTREE_CB_UNSEL,
  MPLTREE_CB_CHANGED,
  MPLTREE_CB_MAX,
};

typedef struct managepltree {
  uitree_t          *uitree;
  UIWidget          *statusMsg;
  UIWidget          uihideunsel;
  callback_t        *callbacks [MPLTREE_CB_MAX];
  playlist_t        *playlist;
  bool              changed : 1;
  bool              hideunselected : 1;
  bool              inprepop : 1;
} managepltree_t;

static void managePlaylistTreeSetColumnVisibility (managepltree_t *managepltree, int pltype);
static bool managePlaylistTreeChanged (void *udata, long col);
static void managePlaylistTreeCreate (managepltree_t *managepltree);
static bool managePlaylistTreeHideUnselectedCallback (void *udata);

managepltree_t *
managePlaylistTreeAlloc (UIWidget *statusMsg)
{
  managepltree_t *managepltree;

  managepltree = mdmalloc (sizeof (managepltree_t));
  managepltree->uitree = NULL;
  managepltree->statusMsg = statusMsg;
  managepltree->playlist = NULL;
  managepltree->changed = false;
  managepltree->hideunselected = false;
  managepltree->inprepop = false;
  for (int i = 0; i < MPLTREE_CB_MAX; ++i) {
    managepltree->callbacks [i] = NULL;
  }

  return managepltree;
}

void
managePlaylistTreeFree (managepltree_t *managepltree)
{
  if (managepltree != NULL) {
    uiTreeViewFree (managepltree->uitree);
    for (int i = 0; i < MPLTREE_CB_MAX; ++i) {
      callbackFree (managepltree->callbacks [i]);
    }
    mdfree (managepltree);
  }
}

void
manageBuildUIPlaylistTree (managepltree_t *managepltree, UIWidget *vboxp,
    UIWidget *tophbox)
{
  UIWidget    hbox;
  UIWidget    uiwidget;
  UIWidget    *uitreewidgetp;
  UIWidget    scwindow;
  const char  *bpmstr;
  char        tbuff [100];

  uiCreateHorizBox (&hbox);
  uiBoxPackEnd (tophbox, &hbox);

  /* CONTEXT: playlist management: hide unselected dances */
  uiCreateCheckButton (&uiwidget, _("Hide Unselected"), 0);
  managepltree->callbacks [MPLTREE_CB_UNSEL] = callbackInit (
      managePlaylistTreeHideUnselectedCallback, managepltree, NULL);
  uiToggleButtonSetCallback (&uiwidget, managepltree->callbacks [MPLTREE_CB_UNSEL]);
  uiBoxPackStart (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&managepltree->uihideunsel, &uiwidget);

  uiCreateScrolledWindow (&scwindow, 300);
  uiWidgetExpandVert (&scwindow);
  uiBoxPackStartExpand (vboxp, &scwindow);

  managepltree->uitree = uiCreateTreeView ();
  uitreewidgetp = uiTreeViewGetUIWidget (managepltree->uitree);

  managepltree->callbacks [MPLTREE_CB_CHANGED] = callbackInitLong (
      managePlaylistTreeChanged, managepltree);
  uiTreeViewSetEditedCallback (managepltree->uitree,
      managepltree->callbacks [MPLTREE_CB_CHANGED]);

  uiWidgetExpandVert (uitreewidgetp);
  uiBoxPackInWindow (&scwindow, uitreewidgetp);

  uiTreeViewEnableHeaders (managepltree->uitree);

  uiTreeViewAppendColumn (managepltree->uitree, TREE_WIDGET_CHECKBOX,
      TREE_COL_DISP_NORM, NULL,
      TREE_COL_MODE_ACTIVE, MPLTREE_COL_DANCE_SELECT,
      TREE_COL_MODE_END);

  uiTreeViewAppendColumn (managepltree->uitree, TREE_WIDGET_TEXT,
      TREE_COL_DISP_GROW, tagdefs [TAG_DANCE].displayname,
      TREE_COL_MODE_TEXT, MPLTREE_COL_DANCE,
      TREE_COL_MODE_END);

  uiTreeViewAppendColumn (managepltree->uitree, TREE_WIDGET_SPINBOX,
      /* CONTEXT: playlist management: count column header */
      TREE_COL_DISP_GROW, _("Count"),
      TREE_COL_MODE_TEXT, MPLTREE_COL_COUNT,
      TREE_COL_MODE_EDITABLE, MPLTREE_COL_EDITABLE,
      TREE_COL_MODE_ADJUSTMENT, MPLTREE_COL_ADJUST,
      TREE_COL_MODE_DIGITS, MPLTREE_COL_DIGITS,
      TREE_COL_MODE_END);

  uiTreeViewAppendColumn (managepltree->uitree, TREE_WIDGET_TEXT,
      /* CONTEXT: playlist management: max play time column header (keep short) */
      TREE_COL_DISP_GROW, _("Maximum\nPlay Time"),
      TREE_COL_MODE_TEXT, MPLTREE_COL_MAXPLAYTIME,
      TREE_COL_MODE_EDITABLE, MPLTREE_COL_EDITABLE,
      TREE_COL_MODE_END);

  bpmstr = tagdefs [TAG_BPM].displayname;

  /* CONTEXT: playlist management: low bpm/mpm column header */
  snprintf (tbuff, sizeof (tbuff), _("Low %s"), bpmstr);
  uiTreeViewAppendColumn (managepltree->uitree, TREE_WIDGET_SPINBOX,
      TREE_COL_DISP_GROW, tbuff,
      TREE_COL_MODE_TEXT, MPLTREE_COL_LOWBPM,
      TREE_COL_MODE_EDITABLE, MPLTREE_COL_EDITABLE,
      TREE_COL_MODE_ADJUSTMENT, MPLTREE_COL_ADJUST,
      TREE_COL_MODE_DIGITS, MPLTREE_COL_DIGITS,
      TREE_COL_MODE_END);

  /* CONTEXT: playlist management: high bpm/mpm column header */
  snprintf (tbuff, sizeof (tbuff), _("High %s"), bpmstr);
  uiTreeViewAppendColumn (managepltree->uitree, TREE_WIDGET_SPINBOX,
      TREE_COL_DISP_GROW, tbuff,
      TREE_COL_MODE_TEXT, MPLTREE_COL_LOWBPM,
      TREE_COL_MODE_EDITABLE, MPLTREE_COL_EDITABLE,
      TREE_COL_MODE_ADJUSTMENT, MPLTREE_COL_ADJUST,
      TREE_COL_MODE_DIGITS, MPLTREE_COL_DIGITS,
      TREE_COL_MODE_END);

  uiTreeViewAppendColumn (managepltree->uitree, TREE_WIDGET_TEXT,
      TREE_COL_DISP_NORM, NULL,
      TREE_COL_MODE_TEXT, MPLTREE_COL_SB_PAD,
      TREE_COL_MODE_END);

  managePlaylistTreeCreate (managepltree);
}

void
managePlaylistTreePrePopulate (managepltree_t *managepltree, playlist_t *pl)
{
  int     pltype;
  int     hideunselstate = UI_TOGGLE_BUTTON_OFF;
  int     widgetstate = UIWIDGET_DISABLE;

  managepltree->playlist = pl;
  managepltree->inprepop = true;
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
  uiWidgetSetState (&managepltree->uihideunsel, widgetstate);
  uiToggleButtonSetState (&managepltree->uihideunsel, hideunselstate);
  managepltree->inprepop = false;
}

void
managePlaylistTreePopulate (managepltree_t *managepltree, playlist_t *pl)
{
  dance_t       *dances;
  int           pltype;
  ilistidx_t    iteridx;
  ilistidx_t    dkey;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  int           count;
  char          tbuff [40];
  UIWidget      *uiwidgetp;

  if (managepltree->uitree == NULL) {
    return;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  managepltree->playlist = pl;
  pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);

  managePlaylistTreeSetColumnVisibility (managepltree, pltype);

  uiwidgetp = uiTreeViewGetUIWidget (managepltree->uitree);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidgetp->widget));

  danceStartIterator (dances, &iteridx);
  count = 0;
  while ((dkey = danceIterate (dances, &iteridx)) >= 0) {
    long  sel, dcount, bpmlow, bpmhigh, mpt;
    char  mptdisp [40];

    sel = playlistGetDanceNum (pl, dkey, PLDANCE_SELECTED);

    if (managepltree->hideunselected && pl != NULL) {
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
    bpmlow = playlistGetDanceNum (pl, dkey, PLDANCE_BPM_LOW);
    if (bpmlow < 0) { bpmlow = 0; }
    bpmhigh = playlistGetDanceNum (pl, dkey, PLDANCE_BPM_HIGH);
    if (bpmhigh < 0) { bpmhigh = 0; }

    snprintf (tbuff, sizeof (tbuff), "%d", count);
    if (gtk_tree_model_get_iter_from_string (model, &iter, tbuff)) {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
          MPLTREE_COL_DANCE_SELECT, (glong) sel,
          MPLTREE_COL_MAXPLAYTIME, mptdisp,
          MPLTREE_COL_COUNT, (glong) dcount,
          MPLTREE_COL_LOWBPM, (glong) bpmlow,
          MPLTREE_COL_HIGHBPM, (glong) bpmhigh,
          -1);
    }
    ++count;
  }

  managepltree->changed = false;
}


bool
managePlaylistTreeIsChanged (managepltree_t *managepltree)
{
  return managepltree->changed;
}

void
managePlaylistTreeUpdatePlaylist (managepltree_t *managepltree)
{
  GtkTreeModel    *model;
  GtkTreeIter     iter;
  glong            tval;
  char            *tstr;
  char            tbuff [40];
  ilistidx_t      dkey;
  playlist_t      *pl;
  int             count;
  UIWidget        *uiwidgetp;

  if (! managepltree->changed) {
    return;
  }

  pl = managepltree->playlist;

  /* hide unselected may be on, and only the displayed dances will */
  /* be updated */

  uiwidgetp = uiTreeViewGetUIWidget (managepltree->uitree);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidgetp->widget));
  count = 0;
  while (1) {
    snprintf (tbuff, sizeof (tbuff), "%d", count);
    if (! gtk_tree_model_get_iter_from_string (model, &iter, tbuff)) {
      break;
    }

    gtk_tree_model_get (model, &iter, MPLTREE_COL_DANCE_IDX, &tval, -1);
    dkey = tval;
    gtk_tree_model_get (model, &iter, MPLTREE_COL_DANCE_SELECT, &tval, -1);
    playlistSetDanceNum (pl, dkey, PLDANCE_SELECTED, tval);
    gtk_tree_model_get (model, &iter, MPLTREE_COL_COUNT, &tval, -1);
    playlistSetDanceNum (pl, dkey, PLDANCE_COUNT, tval);
    gtk_tree_model_get (model, &iter, MPLTREE_COL_MAXPLAYTIME, &tstr, -1);
    tval = tmutilStrToMS (tstr);
    playlistSetDanceNum (pl, dkey, PLDANCE_MAXPLAYTIME, tval);
    gtk_tree_model_get (model, &iter, MPLTREE_COL_LOWBPM, &tval, -1);
    playlistSetDanceNum (pl, dkey, PLDANCE_BPM_LOW, tval);
    gtk_tree_model_get (model, &iter, MPLTREE_COL_HIGHBPM, &tval, -1);
    playlistSetDanceNum (pl, dkey, PLDANCE_BPM_HIGH, tval);

    ++count;
  }
}

/* internal routines */

static void
managePlaylistTreeSetColumnVisibility (managepltree_t *managepltree, int pltype)
{
  switch (pltype) {
    case PLTYPE_SONGLIST: {
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_DANCE_SELECT, TREE_COLUMN_HIDDEN);
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_COUNT, TREE_COLUMN_HIDDEN);
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_LOWBPM, TREE_COLUMN_HIDDEN);
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_HIGHBPM, TREE_COLUMN_HIDDEN);
      break;
    }
    case PLTYPE_AUTO: {
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_DANCE_SELECT, TREE_COLUMN_SHOWN);
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_COUNT, TREE_COLUMN_SHOWN);
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_LOWBPM, TREE_COLUMN_SHOWN);
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_HIGHBPM, TREE_COLUMN_SHOWN);
      break;
    }
    case PLTYPE_SEQUENCE: {
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_DANCE_SELECT, TREE_COLUMN_HIDDEN);
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_COUNT, TREE_COLUMN_HIDDEN);
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_LOWBPM, TREE_COLUMN_SHOWN);
      uiTreeViewColumnSetVisible (managepltree->uitree,
          MPLTREE_COL_HIGHBPM, TREE_COLUMN_SHOWN);
      break;
    }
  }
}

static bool
managePlaylistTreeChanged (void *udata, long col)
{
  managepltree_t  *managepltree = udata;
  bool            rc = UICB_CONT;

  /* force the selection iter to be updated */
  uiTreeViewSelectGetCount (managepltree->uitree);

  uiLabelSetText (managepltree->statusMsg, "");

  if (col == MPLTREE_COL_MAXPLAYTIME) {
    const char  *valstr;
    char        tbuff [200];
    char        *str;

    str = uiTreeViewGetValueStr (managepltree->uitree, MPLTREE_COL_MAXPLAYTIME);
    valstr = validate (str, VAL_MIN_SEC);
    if (valstr == NULL) {
      managepltree->changed = true;
    } else {
      snprintf (tbuff, sizeof (tbuff), valstr, str);
      uiLabelSetText (managepltree->statusMsg, tbuff);
      rc = UICB_STOP;
    }
    dataFree (str);
  }

  if (col == MPLTREE_COL_DANCE_SELECT) {
    int         val;
    ilistidx_t  dkey;

    val = uiTreeViewGetValue (managepltree->uitree, MPLTREE_COL_DANCE_SELECT);
    dkey = uiTreeViewGetValue (managepltree->uitree, MPLTREE_COL_DANCE_IDX);
    playlistSetDanceNum (managepltree->playlist, dkey, PLDANCE_SELECTED, val);
  }

  managepltree->changed = true;
  return rc;
}

static void
managePlaylistTreeCreate (managepltree_t *managepltree)
{
  dance_t       *dances;
  GtkTreeIter   iter;
  slist_t       *dancelist;
  slistidx_t    iteridx;
  ilistidx_t    key;
  uitree_t      *uitree;
  UIWidget      *uiwidgetp;
  GtkTreeModel  *model;
  GtkListStore  *store;
  GtkAdjustment *adjustment;

  uitree = managepltree->uitree;

  uiTreeViewCreateValueStore (uitree, MPLTREE_COL_MAX,
      TREE_TYPE_BOOLEAN,  // dance select
      TREE_TYPE_STRING,   // dance
      TREE_TYPE_NUM,      // count
      TREE_TYPE_STRING,   // max play time
      TREE_TYPE_NUM,      // low bpm
      TREE_TYPE_NUM,      // high bpm
      TREE_TYPE_STRING,   // pad
      TREE_TYPE_NUM,      // dance idx
      TREE_TYPE_NUM,      // editable
      TREE_TYPE_WIDGET,   // adjust
      TREE_TYPE_NUM,      // digits
      TREE_TYPE_END);

  uiwidgetp = uiTreeViewGetUIWidget (uitree);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidgetp->widget));
  store = GTK_LIST_STORE (model);

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dancelist = danceGetDanceList (dances);

  slistStartIterator (dancelist, &iteridx);
  while ((key = slistIterateValueNum (dancelist, &iteridx)) >= 0) {
    char        *dancedisp;

    dancedisp = danceGetStr (dances, key, DANCE_DANCE);

    if (managepltree->hideunselected &&
        managepltree->playlist != NULL) {
      int     sel;

      sel = playlistGetDanceNum (managepltree->playlist, key, PLDANCE_SELECTED);
      if (! sel) {
        /* don't display this one */
        continue;
      }
    }

    gtk_list_store_append (store, &iter);
    adjustment = gtk_adjustment_new (0, 0.0, 200.0, 1.0, 5.0, 0.0);
    gtk_list_store_set (store, &iter,
        MPLTREE_COL_DANCE_SELECT, 0,
        MPLTREE_COL_DANCE, dancedisp,
        MPLTREE_COL_MAXPLAYTIME, "0:00",
        MPLTREE_COL_LOWBPM, 0,
        MPLTREE_COL_HIGHBPM, 0,
        MPLTREE_COL_SB_PAD, "  ",
        MPLTREE_COL_DANCE_IDX, key,
        MPLTREE_COL_EDITABLE, 1,
        MPLTREE_COL_ADJUST, adjustment,
        MPLTREE_COL_DIGITS, 0,
        -1);
  }

  uiwidgetp = uiTreeViewGetUIWidget (uitree);
}

static bool
managePlaylistTreeHideUnselectedCallback (void *udata)
{
  managepltree_t  *managepltree = udata;
  bool            tchg;

  tchg = managepltree->changed;
  if (managepltree->inprepop) {
    managepltree->hideunselected =
        uiToggleButtonIsActive (&managepltree->uihideunsel);
  }
  if (! managepltree->inprepop) {
    managepltree->hideunselected = ! managepltree->hideunselected;
  }
  managePlaylistTreeCreate (managepltree);
  managePlaylistTreePopulate (managepltree, managepltree->playlist);
  managepltree->changed = tchg;
  return UICB_CONT;
}

