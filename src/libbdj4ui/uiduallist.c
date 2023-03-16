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

#include <gtk/gtk.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "istring.h"
#include "mdebug.h"
#include "slist.h"
#include "ui.h"
#include "callback.h"
#include "uiduallist.h"

enum {
  DUALLIST_COL_DISP,
  DUALLIST_COL_SB_PAD,
  DUALLIST_COL_DISP_IDX,
  DUALLIST_COL_MAX,
};

enum {
  DUALLIST_MOVE_PREV = -1,
  DUALLIST_MOVE_NEXT = 1,
};

enum {
  DUALLIST_SEARCH_INSERT,
  DUALLIST_SEARCH_REMOVE,
};

enum {
  DUALLIST_BUTTON_SELECT,
  DUALLIST_BUTTON_REMOVE,
  DUALLIST_BUTTON_MOVE_UP,
  DUALLIST_BUTTON_MOVE_DOWN,
  DUALLIST_BUTTON_MAX,
};

typedef struct {
  uitree_t          *uitree;
  GtkTreeSelection  *sel;
} uiduallisttree_t;

enum {
  DUALLIST_CB_MOVEPREV,
  DUALLIST_CB_MOVENEXT,
  DUALLIST_CB_SELECT,
  DUALLIST_CB_REMOVE,
  DUALLIST_CB_MAX,
};

typedef struct uiduallist {
  uiduallisttree_t  trees [DUALLIST_TREE_MAX];
  callback_t        *callbacks [DUALLIST_CB_MAX];
  uibutton_t        *buttons [DUALLIST_BUTTON_MAX];
  int               flags;
  char              *searchstr;
  int               pos;
  int               searchtype;
  slist_t           *savelist;
  bool              changed : 1;
} uiduallist_t;

static bool uiduallistMovePrev (void *tduallist);
static bool uiduallistMoveNext (void *tduallist);
static void uiduallistMove (uiduallist_t *duallist, int which, int dir);
static bool uiduallistDispSelect (void *udata);
static bool uiduallistDispRemove (void *udata);
static gboolean uiduallistSourceSearch (GtkTreeModel* model,
    GtkTreePath* path, GtkTreeIter* iter, gpointer udata);
static gboolean uiduallistGetData (GtkTreeModel* model, GtkTreePath* path,
    GtkTreeIter* iter, gpointer udata);
static void uiduallistSetDefaultSelection (uiduallist_t *duallist, int which);

uiduallist_t *
uiCreateDualList (UIWidget *mainvbox, int flags,
    const char *sourcetitle, const char *targettitle)
{
  uiduallist_t  *duallist;
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      dvbox;
  UIWidget      uiwidget;
  UIWidget      scwindow;
  uibutton_t    *uibutton;
  UIWidget      *uiwidgetp = NULL;
  uitree_t      *uitree;
  UIWidget      *uitreewidgetp = NULL;

  duallist = mdmalloc (sizeof (uiduallist_t));
  for (int i = 0; i < DUALLIST_TREE_MAX; ++i) {
    duallist->trees [i].uitree = NULL;
    duallist->trees [i].sel = NULL;
  }
  duallist->flags = flags;
  duallist->pos = 0;
  duallist->searchtype = DUALLIST_SEARCH_INSERT;
  duallist->searchstr = NULL;
  duallist->savelist = NULL;
  duallist->changed = false;
  for (int i = 0; i < DUALLIST_BUTTON_MAX; ++i) {
    duallist->buttons [i] = NULL;
  }
  for (int i = 0; i < DUALLIST_CB_MAX; ++i) {
    duallist->callbacks [i] = NULL;
  }

  duallist->callbacks [DUALLIST_CB_MOVEPREV] = callbackInit (
      uiduallistMovePrev, duallist, NULL);
  duallist->callbacks [DUALLIST_CB_MOVENEXT] = callbackInit (
      uiduallistMoveNext, duallist, NULL);
  duallist->callbacks [DUALLIST_CB_SELECT] = callbackInit (
      uiduallistDispSelect, duallist, NULL);
  duallist->callbacks [DUALLIST_CB_REMOVE] = callbackInit (
      uiduallistDispRemove, duallist, NULL);

  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&dvbox);
  uiutilsUIWidgetInit (&uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetAlignHorizStart (&hbox);
  uiBoxPackStartExpand (mainvbox, &hbox);

  uiCreateVertBox (&vbox);
  uiWidgetSetMarginStart (&vbox, 8);
  uiWidgetSetMarginTop (&vbox, 8);
  uiBoxPackStartExpand (&hbox, &vbox);

  if (sourcetitle != NULL) {
    uiCreateLabel (&uiwidget, sourcetitle);
    uiBoxPackStart (&vbox, &uiwidget);
  }

  uiCreateScrolledWindow (&scwindow, 300);
  uiWidgetExpandVert (&scwindow);
  uiBoxPackStartExpand (&vbox, &scwindow);

  uitree = uiCreateTreeView ();
  duallist->trees [DUALLIST_TREE_SOURCE].uitree = uitree;
  uitreewidgetp = uiTreeViewGetUIWidget (uitree);
  uiTreeViewDarkBackground (uitree);
  uiWidgetExpandVert (uitreewidgetp);
  uiBoxPackInWindow (&scwindow, uitreewidgetp);
  duallist->trees [DUALLIST_TREE_SOURCE].sel =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (uitreewidgetp->widget));

  uiTreeViewCreateValueStore (uitree, DUALLIST_COL_MAX,
      TREE_TYPE_STRING, TREE_TYPE_STRING, TREE_TYPE_NUM, TREE_TYPE_END);
  uiTreeViewDisableHeaders (uitree);

  uiTreeViewAppendColumn (uitree, TREE_COL_DISP_GROW, "",
      TREE_COL_MODE_TEXT, DUALLIST_COL_DISP, TREE_COL_MODE_END);
  uiTreeViewAppendColumn (uitree, TREE_COL_DISP_NORM, "",
      TREE_COL_MODE_TEXT, DUALLIST_COL_SB_PAD, TREE_COL_MODE_END);

  uiCreateVertBox (&dvbox);
  uiWidgetSetAllMargins (&dvbox, 4);
  uiWidgetSetMarginTop (&dvbox, 64);
  uiWidgetAlignVertStart (&dvbox);
  uiBoxPackStart (&hbox, &dvbox);

  uibutton = uiCreateButton (duallist->callbacks [DUALLIST_CB_SELECT],
      /* CONTEXT: side-by-side list: button: add the selected field */
      _("Select"), "button_right");
  duallist->buttons [DUALLIST_BUTTON_SELECT] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&dvbox, uiwidgetp);

  uibutton = uiCreateButton (duallist->callbacks [DUALLIST_CB_REMOVE],
      /* CONTEXT: side-by-side list: button: remove the selected field */
      _("Remove"), "button_left");
  duallist->buttons [DUALLIST_BUTTON_REMOVE] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&dvbox, uiwidgetp);

  uiCreateVertBox (&vbox);
  uiWidgetSetMarginStart (&vbox, 8);
  uiWidgetSetMarginTop (&vbox, 8);
  uiBoxPackStartExpand (&hbox, &vbox);

  if (targettitle != NULL) {
    uiCreateLabel (&uiwidget, targettitle);
    uiBoxPackStart (&vbox, &uiwidget);
  }

  uiCreateScrolledWindow (&scwindow, 300);
  uiWidgetExpandVert (&scwindow);
  uiBoxPackStartExpand (&vbox, &scwindow);

  uitree = uiCreateTreeView ();
  duallist->trees [DUALLIST_TREE_TARGET].uitree = uitree;
  uitreewidgetp = uiTreeViewGetUIWidget (uitree);
  uiTreeViewDarkBackground (uitree);
  uiWidgetExpandVert (uitreewidgetp);
  uiBoxPackInWindow (&scwindow, uitreewidgetp);
  duallist->trees [DUALLIST_TREE_TARGET].sel =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (uitreewidgetp->widget));

  uiTreeViewCreateValueStore (uitree, DUALLIST_COL_MAX,
      TREE_TYPE_STRING, TREE_TYPE_STRING, TREE_TYPE_NUM, TREE_TYPE_END);
  uiTreeViewDisableHeaders (uitree);

  uiTreeViewAppendColumn (uitree, TREE_COL_DISP_GROW, "",
      TREE_COL_MODE_TEXT, DUALLIST_COL_DISP, -1);
  uiTreeViewAppendColumn (uitree, TREE_COL_DISP_NORM, "",
      TREE_COL_MODE_TEXT, DUALLIST_COL_SB_PAD, -1);

  uiCreateVertBox (&dvbox);
  uiWidgetSetAllMargins (&dvbox, 4);
  uiWidgetSetMarginTop (&dvbox, 64);
  uiWidgetAlignVertStart (&dvbox);
  uiBoxPackStart (&hbox, &dvbox);

  uibutton = uiCreateButton (duallist->callbacks [DUALLIST_CB_MOVEPREV],
      /* CONTEXT: side-by-side list: button: move the selected field up */
      _("Move Up"), "button_up");
  duallist->buttons [DUALLIST_BUTTON_MOVE_UP] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&dvbox, uiwidgetp);

  uibutton = uiCreateButton (duallist->callbacks [DUALLIST_CB_MOVENEXT],
      /* CONTEXT: side-by-side list: button: move the selected field down */
      _("Move Down"), "button_down");
  duallist->buttons [DUALLIST_BUTTON_MOVE_DOWN] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&dvbox, uiwidgetp);

  return duallist;
}

void
uiduallistFree (uiduallist_t *duallist)
{
  if (duallist != NULL) {
    for (int i = 0; i < DUALLIST_TREE_MAX; ++i) {
      uiTreeViewFree (duallist->trees [i].uitree);
    }
    for (int i = 0; i < DUALLIST_CB_MAX; ++i) {
      callbackFree (duallist->callbacks [i]);
    }
    for (int i = 0; i < DUALLIST_BUTTON_MAX; ++i) {
      uiButtonFree (duallist->buttons [i]);
    }
    mdfree (duallist);
  }
}


void
uiduallistSet (uiduallist_t *duallist, slist_t *slist, int which)
{
  UIWidget      *uiwidgetp = NULL;
  char          *keystr;
  slistidx_t    siteridx;
  char          tmp [40];
  uitree_t      *uitree = NULL;


  if (duallist == NULL) {
    return;
  }

  if (which < 0 || which >= DUALLIST_TREE_MAX) {
    return;
  }

  uitree = duallist->trees [which].uitree;
  uiwidgetp = uiTreeViewGetUIWidget (uitree);
  if (uiwidgetp->widget == NULL) {
    return;
  }

  /* the assumption made is that the source tree has been populated */
  /* just before the target tree */

  uiTreeViewCreateValueStore (uitree, DUALLIST_COL_MAX,
      TREE_TYPE_STRING, TREE_TYPE_STRING, TREE_TYPE_NUM, TREE_TYPE_END);

  slistStartIterator (slist, &siteridx);
  while ((keystr = slistIterateKey (slist, &siteridx)) != NULL) {
    long    val;

    val = slistGetNum (slist, keystr);
    uiTreeViewAppendValueStore (uitree);
    uiTreeViewSetValues (uitree,
        DUALLIST_COL_DISP, keystr,
        DUALLIST_COL_SB_PAD, "    ",
        DUALLIST_COL_DISP_IDX, val,
        TREE_VALUE_END);

    /* if inserting into the target tree, and the persistent flag */
    /* is not set, remove the matching entries from the source tree */
    if (which == DUALLIST_TREE_TARGET &&
        (duallist->flags & DUALLIST_FLAGS_PERSISTENT) != DUALLIST_FLAGS_PERSISTENT) {
      UIWidget          *stree;
      GtkTreeModel      *smodel;
      GtkTreePath       *path;
      GtkTreeIter       siter;

      stree = uiTreeViewGetUIWidget (duallist->trees [DUALLIST_TREE_SOURCE].uitree);
      smodel = gtk_tree_view_get_model (GTK_TREE_VIEW (stree->widget));

      duallist->pos = 0;
      duallist->searchstr = keystr;
      duallist->searchtype = DUALLIST_SEARCH_REMOVE;
      /* this is not efficient, but the lists are relatively short */
      gtk_tree_model_foreach (smodel, uiduallistSourceSearch, duallist);

      snprintf (tmp, sizeof (tmp), "%d", duallist->pos);
      path = gtk_tree_path_new_from_string (tmp);
      mdextalloc (path);
      if (path != NULL) {
        if (gtk_tree_model_get_iter (smodel, &siter, path)) {
          gtk_list_store_remove (GTK_LIST_STORE (smodel), &siter);
        }
        mdextfree (path);
        gtk_tree_path_free (path);
      }
    }
  }

  uiduallistSetDefaultSelection (duallist, which);
}

bool
uiduallistIsChanged (uiduallist_t *duallist)
{
  if (duallist == NULL) {
    return false;
  }

  return duallist->changed;
}

void
uiduallistClearChanged (uiduallist_t *duallist)
{
  if (duallist == NULL) {
    return;
  }

  duallist->changed = false;
}

slist_t *
uiduallistGetList (uiduallist_t *duallist)
{
  slist_t       *slist;
  UIWidget      *ttree;
  GtkTreeModel  *tmodel;


  ttree = uiTreeViewGetUIWidget (duallist->trees [DUALLIST_TREE_TARGET].uitree);
  tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (ttree->widget));

  slist = slistAlloc ("duallist-return", LIST_UNORDERED, NULL);
  duallist->savelist = slist;
  gtk_tree_model_foreach (tmodel, uiduallistGetData, duallist);
  return slist;
}


/* internal routines */

static bool
uiduallistMovePrev (void *tduallist)
{
  uiduallist_t  *duallist = tduallist;
  uiduallistMove (duallist, DUALLIST_TREE_TARGET, DUALLIST_MOVE_PREV);
  return UICB_CONT;
}

static bool
uiduallistMoveNext (void *tduallist)
{
  uiduallist_t  *duallist = tduallist;
  uiduallistMove (duallist, DUALLIST_TREE_TARGET, DUALLIST_MOVE_NEXT);
  return UICB_CONT;
}

static void
uiduallistMove (uiduallist_t *duallist, int which, int dir)
{
  UIWidget          *tree;
  GtkTreeSelection  *sel;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GtkTreeIter       citer;
  GtkTreePath       *path;
  char              *pathstr;
  int               count;
  int               valid;
  int               idx;

  tree = uiTreeViewGetUIWidget (duallist->trees [which].uitree);
  if (tree->widget == NULL) {
    return;
  }

  sel = duallist->trees [which].sel;
  count = uiTreeViewSelectionGetCount (duallist->trees [which].uitree);
  if (count != 1) {
    return;
  }
  valid = gtk_tree_selection_get_selected (sel, &model, &iter);
  if (! valid) {
    return;
  }

  idx = 0;
  path = gtk_tree_model_get_path (model, &iter);
  mdextalloc (path);
  if (path != NULL) {
    pathstr = gtk_tree_path_to_string (path);
    mdextalloc (pathstr);
    sscanf (pathstr, "%d", &idx);
    mdextfree (path);
    gtk_tree_path_free (path);
    mdfree (pathstr);         // allocated by gtk
  }

  memcpy (&citer, &iter, sizeof (iter));
  if (dir == DUALLIST_MOVE_PREV) {
    valid = gtk_tree_model_iter_previous (model, &iter);
    if (valid) {
      gtk_list_store_move_before (GTK_LIST_STORE (model), &citer, &iter);
    }
  } else {
    valid = gtk_tree_model_iter_next (model, &iter);
    if (valid) {
      gtk_list_store_move_after (GTK_LIST_STORE (model), &citer, &iter);
    }
  }
  duallist->changed = true;
}

static bool
uiduallistDispSelect (void *udata)
{
  uiduallist_t      *duallist = udata;
  UIWidget          *ttree;
  GtkTreeSelection  *tsel;
  uitree_t          *uistree;
  GtkTreeModel      *smodel;
  GtkTreeIter       siter;
  int               count;
  uitree_t          *uittree;
  char              *str;
  glong             tval;
  GtkTreeModel      *tmodel;
  GtkTreeIter       titer;
  GtkTreeIter       lociter;
  GtkTreeIter       *lociterp;
  GtkTreePath       *path;

  uistree = duallist->trees [DUALLIST_TREE_SOURCE].uitree;

  count = uiTreeViewGetSelection (uistree, &smodel, &siter);

  if (count != 1) {
    return UICB_CONT;
  }

  uittree = duallist->trees [DUALLIST_TREE_TARGET].uitree;
  ttree = uiTreeViewGetUIWidget (uittree);
  tsel = duallist->trees [DUALLIST_TREE_TARGET].sel;
  tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (ttree->widget));

  lociterp = &lociter;
  count = uiTreeViewGetSelection (uittree, &tmodel, lociterp);
  if (count == 0) {
    lociterp = NULL;
  }

  gtk_tree_model_get (smodel, &siter, DUALLIST_COL_DISP, &str, -1);
  gtk_tree_model_get (smodel, &siter, DUALLIST_COL_DISP_IDX, &tval, -1);

  gtk_list_store_insert_after (GTK_LIST_STORE (tmodel), &titer, lociterp);
  gtk_list_store_set (GTK_LIST_STORE (tmodel), &titer,
      DUALLIST_COL_DISP, str,
      DUALLIST_COL_SB_PAD, "    ",
      DUALLIST_COL_DISP_IDX, tval,
      -1);

  path = gtk_tree_model_get_path (tmodel, &titer);
  mdextalloc (path);
  if (path != NULL) {
    gtk_tree_selection_select_path (tsel, path);
    mdextfree (path);
    gtk_tree_path_free (path);
  }

  if ((duallist->flags & DUALLIST_FLAGS_PERSISTENT) != DUALLIST_FLAGS_PERSISTENT) {
    gtk_list_store_remove (GTK_LIST_STORE (smodel), &siter);
  }
  duallist->changed = true;
  return UICB_CONT;
}

static bool
uiduallistDispRemove (void *udata)
{
  uiduallist_t  *duallist = udata;
  uitree_t      *uittree;
  GtkTreeSelection *tsel;
  GtkTreeModel  *tmodel;
  GtkTreeIter   piter;
  GtkTreeIter   titer;
  GtkTreePath   *path;
  int           count;
  char          *str;
  glong         tval;
  UIWidget      *stree;
  GtkTreeSelection *ssel;
  GtkTreeModel  *smodel;
  GtkTreeIter   siter;


  uittree = duallist->trees [DUALLIST_TREE_TARGET].uitree;
  tsel = duallist->trees [DUALLIST_TREE_TARGET].sel;
  count = uiTreeViewGetSelection (uittree, &tmodel, &titer);

  if (count != 1) {
    return UICB_CONT;
  }

  if ((duallist->flags & DUALLIST_FLAGS_PERSISTENT) != DUALLIST_FLAGS_PERSISTENT) {
    stree = uiTreeViewGetUIWidget (duallist->trees [DUALLIST_TREE_SOURCE].uitree);
    ssel = duallist->trees [DUALLIST_TREE_SOURCE].sel;
    smodel = gtk_tree_view_get_model (GTK_TREE_VIEW (stree->widget));

    gtk_tree_model_get (tmodel, &titer, DUALLIST_COL_DISP, &str, -1);
    gtk_tree_model_get (tmodel, &titer, DUALLIST_COL_DISP_IDX, &tval, -1);

    duallist->pos = 0;
    duallist->searchstr = str;
    duallist->searchtype = DUALLIST_SEARCH_INSERT;
    gtk_tree_model_foreach (smodel, uiduallistSourceSearch, duallist);

    gtk_list_store_insert (GTK_LIST_STORE (smodel), &siter, duallist->pos);
    gtk_list_store_set (GTK_LIST_STORE (smodel), &siter,
        DUALLIST_COL_DISP, str,
        DUALLIST_COL_SB_PAD, "    ",
        DUALLIST_COL_DISP_IDX, tval,
        -1);

    path = gtk_tree_model_get_path (smodel, &siter);
    mdextalloc (path);
    if (path != NULL) {
      gtk_tree_selection_select_path (ssel, path);
      mdextfree (path);
      gtk_tree_path_free (path);
    }
  }

  path = NULL;

  /* only select the previous if the current iter is at the last */
  memcpy (&piter, &titer, sizeof (GtkTreeIter));
  if (! gtk_tree_model_iter_next (tmodel, &piter)) {
    memcpy (&piter, &titer, sizeof (GtkTreeIter));
    if (gtk_tree_model_iter_previous (tmodel, &piter)) {
      path = gtk_tree_model_get_path (tmodel, &piter);
      mdextalloc (path);
    }
  }

  gtk_list_store_remove (GTK_LIST_STORE (tmodel), &titer);

  if (path != NULL) {
    gtk_tree_selection_select_path (tsel, path);
    mdextfree (path);
    gtk_tree_path_free (path);
  }

  duallist->changed = true;
  return UICB_CONT;
}

static gboolean
uiduallistSourceSearch (GtkTreeModel* model, GtkTreePath* path,
    GtkTreeIter* iter, gpointer udata)
{
  uiduallist_t  *duallist = udata;
  char          *str;

  gtk_tree_model_get (model, iter, DUALLIST_COL_DISP, &str, -1);
  if (duallist->searchtype == DUALLIST_SEARCH_INSERT) {
    if (istringCompare (duallist->searchstr, str) < 0) {
      return TRUE;
    }
  }
  if (duallist->searchtype == DUALLIST_SEARCH_REMOVE) {
    if (istringCompare (duallist->searchstr, str) == 0) {
      return TRUE;
    }
  }

  duallist->pos += 1;
  return FALSE; // continue iterating
}

static gboolean
uiduallistGetData (GtkTreeModel* model, GtkTreePath* path,
    GtkTreeIter* iter, gpointer udata)
{
  uiduallist_t  *duallist = udata;
  char          *str;
  glong         tval;

  gtk_tree_model_get (model, iter, DUALLIST_COL_DISP, &str, -1);
  gtk_tree_model_get (model, iter, DUALLIST_COL_DISP_IDX, &tval, -1);
  slistSetNum (duallist->savelist, str, tval);
  return FALSE; // continue iterating
}


static void
uiduallistSetDefaultSelection (uiduallist_t *duallist, int which)
{
  int               count;
  uitree_t          *uitree;
  GtkTreeIter       iter;
  GtkTreeModel      *model;

  if (duallist == NULL) {
    return;
  }
  if (which >= DUALLIST_TREE_MAX) {
    return;
  }

  uitree = duallist->trees [which].uitree;

  count = uiTreeViewGetSelection (uitree, &model, &iter);
  if (count != 1) {
    uiTreeViewSelectionSet (uitree, 0);
  }
}
