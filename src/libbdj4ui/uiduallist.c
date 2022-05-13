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
#include "slist.h"
#include "ui.h"
#include "uiduallist.h"
#include "uiutils.h"

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

static void uiduallistMovePrev (void *tduallist);
static void uiduallistMoveNext (void *tduallist);
static void uiduallistMove (uiduallist_t *duallist, int which, int dir);
static void uiduallistDispSelect (void *udata);
static void uiduallistDispRemove (void *udata);
static gboolean uiduallistSourceSearch (GtkTreeModel* model,
    GtkTreePath* path, GtkTreeIter* iter, gpointer udata);

uiduallist_t *
uiCreateDualList (UIWidget *vbox, int flags)
{
  uiduallist_t  *duallist;
  UIWidget      hbox;
  UIWidget      dvbox;
  UIWidget      uiwidget;
  GtkWidget     *tree;

  duallist = malloc (sizeof (uiduallist_t));
  for (int i = 0; i < DUALLIST_TREE_MAX; ++i) {
    duallist->trees [i].tree = NULL;
    duallist->trees [i].sel = NULL;
  }
  duallist->flags = flags;
  duallist->pos = 0;
  duallist->searchtype = DUALLIST_SEARCH_INSERT;
  duallist->searchstr = NULL;
  duallist->changed = false;

  uiutilsUICallbackInit (&duallist->moveprevcb, uiduallistMovePrev, duallist);
  uiutilsUICallbackInit (&duallist->movenextcb, uiduallistMoveNext, duallist);
  uiutilsUICallbackInit (&duallist->selectcb, uiduallistDispSelect, duallist);
  uiutilsUICallbackInit (&duallist->removecb, uiduallistDispRemove, duallist);

  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&dvbox);
  uiutilsUIWidgetInit (&uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetAlignHorizStart (&hbox);
  uiBoxPackStartExpand (vbox, &hbox);

  uiCreateScrolledWindow (&uiwidget, 300);
  uiWidgetExpandVert (&uiwidget);
  uiBoxPackStartExpand (&hbox, &uiwidget);

  tree = uiCreateTreeView ();
  uiSetCss (tree,
      "treeview { background-color: shade(@theme_base_color,0.8); } "
      "treeview:selected { background-color: @theme_selected_bg_color; } ");
  uiWidgetSetMarginStartW (tree, uiBaseMarginSz * 8);
  uiWidgetSetMarginTopW (tree, uiBaseMarginSz * 8);
  uiWidgetExpandVertW (tree);
  uiBoxPackInWindowUW (&uiwidget, tree);
  duallist->trees [DUALLIST_TREE_SOURCE].tree = tree;
  duallist->trees [DUALLIST_TREE_SOURCE].sel =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  uiCreateVertBox (&dvbox);
  uiWidgetSetAllMargins (&dvbox, uiBaseMarginSz * 4);
  uiWidgetSetMarginTop (&dvbox, uiBaseMarginSz * 64);
  uiWidgetAlignVertStart (&dvbox);
  uiBoxPackStart (&hbox, &dvbox);

  /* CONTEXT: configuration: display settings: button: add the selected field */
  uiCreateButton (&uiwidget, &duallist->selectcb, _("Select"),
      "button_right", NULL, NULL);
  uiBoxPackStart (&dvbox, &uiwidget);

  /* CONTEXT: configuration: display settings: button: remove the selected field */
  uiCreateButton (&uiwidget, &duallist->removecb, _("Remove"),
      "button_left", NULL, NULL);
  uiBoxPackStart (&dvbox, &uiwidget);

  uiCreateScrolledWindow (&uiwidget, 300);
  uiWidgetExpandVert (&uiwidget);
  uiBoxPackStartExpand (&hbox, &uiwidget);

  tree = uiCreateTreeView ();
  uiSetCss (tree,
      "treeview { background-color: shade(@theme_base_color,0.8); } "
      "treeview:selected { background-color: @theme_selected_bg_color; } ");
  uiWidgetSetMarginStartW (tree, uiBaseMarginSz * 8);
  uiWidgetSetMarginTopW (tree, uiBaseMarginSz * 8);
  uiWidgetExpandVertW (tree);
  uiBoxPackInWindowUW (&uiwidget, tree);
  duallist->trees [DUALLIST_TREE_TARGET].tree = tree;
  duallist->trees [DUALLIST_TREE_TARGET].sel =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  uiCreateVertBox (&dvbox);
  uiWidgetSetAllMargins (&dvbox, uiBaseMarginSz * 4);
  uiWidgetSetMarginTop (&dvbox, uiBaseMarginSz * 64);
  uiWidgetAlignVertStart (&dvbox);
  uiBoxPackStart (&hbox, &dvbox);

  /* CONTEXT: configuration: display settings: button: move the selected field up */
  uiCreateButton (&uiwidget, &duallist->moveprevcb, _("Move Up"),
      "button_up", NULL, NULL);
  uiBoxPackStart (&dvbox, &uiwidget);

  /* CONTEXT: configuration: display settings: button: move the selected field down */
  uiCreateButton (&uiwidget, &duallist->movenextcb, _("Move Down"),
      "button_down", NULL, NULL);
  uiBoxPackStart (&dvbox, &uiwidget);

  return duallist;
}

void
uiduallistFree (uiduallist_t *duallist)
{
  if (duallist != NULL) {
    free (duallist);
  }
}


void
uiduallistSet (uiduallist_t *duallist, slist_t *slist, int which)
{
  GtkWidget     *tree;
  GtkListStore  *store;
  GtkTreeIter   iter;
  char          *keystr;
  slistidx_t    siteridx;
  char          tmp [40];


  if (which >= DUALLIST_TREE_MAX) {
    return;
  }

  tree = duallist->trees [which].tree;
  store = gtk_list_store_new (DUALLIST_COL_MAX,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_LONG);
  assert (store != NULL);

  slistStartIterator (slist, &siteridx);
  while ((keystr = slistIterateKey (slist, &siteridx)) != NULL) {
    long    val;

    val = slistGetNum (slist, keystr);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        DUALLIST_COL_DISP, keystr,
        DUALLIST_COL_SB_PAD, "    ",
        DUALLIST_COL_DISP_IDX, val,
        -1);

    /* if inserting into the target tree, and the peristent flag */
    /* is not set, remove the matching entries from the source tree */
    if (which == DUALLIST_TREE_TARGET &&
        (duallist->flags & DUALLIST_FLAGS_PERSISTENT) != DUALLIST_FLAGS_PERSISTENT) {
      GtkWidget         *stree;
      GtkTreeModel      *smodel;

      stree = duallist->trees [DUALLIST_TREE_TARGET].tree;
      smodel = gtk_tree_view_get_model (GTK_TREE_VIEW (stree));

      duallist->pos = 0;
      duallist->searchstr = keystr;
      duallist->searchtype = DUALLIST_SEARCH_REMOVE;
      gtk_tree_model_foreach (smodel, uiduallistSourceSearch, duallist);
      snprintf (tmp, sizeof (tmp), "%d", duallist->pos);
// ### get the path/iter for this position
//      gtk_list_store_remove (GTK_LIST_STORE (smodel), &siter);
    }
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
}

inline bool
uiduallistIsChanged (uiduallist_t *duallist)
{
  if (duallist == NULL) {
    return false;
  }

  return duallist->changed;
}

inline void
uiduallistClearChanged (uiduallist_t *duallist)
{
  if (duallist == NULL) {
    return;
  }

  duallist->changed = false;
}


/* internal routines */

static void
uiduallistMovePrev (void *tduallist)
{
  uiduallist_t  *duallist = tduallist;
  uiduallistMove (duallist, DUALLIST_TREE_TARGET, DUALLIST_MOVE_PREV);
}

static void
uiduallistMoveNext (void *tduallist)
{
  uiduallist_t  *duallist = tduallist;
  uiduallistMove (duallist, DUALLIST_TREE_TARGET, DUALLIST_MOVE_NEXT);
}

static void
uiduallistMove (uiduallist_t *duallist, int which, int dir)
{
  GtkWidget         *tree;
  GtkTreeSelection  *sel;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GtkTreeIter       citer;
  GtkTreePath       *path;
  char              *pathstr;
  int               count;
  gboolean          valid;
  int               idx;

  tree = duallist->trees [which].tree;
  sel = duallist->trees [which].sel;
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);

  path = gtk_tree_model_get_path (model, &iter);
  pathstr = gtk_tree_path_to_string (path);
  sscanf (pathstr, "%d", &idx);
  free (pathstr);
  gtk_tree_path_free (path);

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

static void
uiduallistDispSelect (void *udata)
{
  uiduallist_t      *duallist = udata;
  GtkWidget         *ttree;
  GtkTreeSelection  *tsel;
  GtkWidget         *stree;
  GtkTreeSelection  *ssel;
  GtkTreeModel      *smodel;
  GtkTreeIter       siter;
  int               count;

  stree = duallist->trees [DUALLIST_TREE_SOURCE].tree;
  ssel = duallist->trees [DUALLIST_TREE_SOURCE].sel;

  count = uiTreeViewGetSelection (stree, &smodel, &siter);

  if (count == 1) {
    char          *str;
    glong         tval;
    GtkTreeModel  *tmodel;
    GtkTreeIter   titer;
    GtkTreePath   *path;

    ttree = duallist->trees [DUALLIST_TREE_TARGET].tree;
    tsel = duallist->trees [DUALLIST_TREE_TARGET].sel;
    tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (ttree));

    gtk_tree_model_get (smodel, &siter, DUALLIST_COL_DISP, &str, -1);
    gtk_tree_model_get (smodel, &siter, DUALLIST_COL_DISP_IDX, &tval, -1);

    gtk_list_store_append (GTK_LIST_STORE (tmodel), &titer);
    gtk_list_store_set (GTK_LIST_STORE (tmodel), &titer,
        DUALLIST_COL_DISP, str,
        DUALLIST_COL_SB_PAD, "    ",
        DUALLIST_COL_DISP_IDX, tval,
        -1);

    path = gtk_tree_model_get_path (tmodel, &titer);
    gtk_tree_selection_select_path (tsel, path);

    if ((duallist->flags & DUALLIST_FLAGS_PERSISTENT) != DUALLIST_FLAGS_PERSISTENT) {
      gtk_list_store_remove (GTK_LIST_STORE (smodel), &siter);
    }
    duallist->changed = true;
  }
}

static void
uiduallistDispRemove (void *udata)
{
  uiduallist_t  *duallist = udata;
  GtkWidget     *ttree;
  GtkTreeModel  *tmodel;
  GtkTreeIter   titer;
  int           count;

  ttree = duallist->trees [DUALLIST_TREE_TARGET].tree;
  count = uiTreeViewGetSelection (ttree, &tmodel, &titer);

  if (count == 1) {
    char          *str;
    glong        tval;
    GtkWidget     *stree;
    GtkTreeSelection *ssel;
    GtkTreeModel  *smodel;
    GtkTreeIter   siter;
    GtkTreePath   *path;

    if ((duallist->flags & DUALLIST_FLAGS_PERSISTENT) != DUALLIST_FLAGS_PERSISTENT) {
      stree = duallist->trees [DUALLIST_TREE_SOURCE].tree;
      ssel = duallist->trees [DUALLIST_TREE_SOURCE].sel;
      smodel = gtk_tree_view_get_model (GTK_TREE_VIEW (stree));

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
      gtk_tree_selection_select_path (ssel, path);
    }

    gtk_list_store_remove (GTK_LIST_STORE (tmodel), &titer);
    duallist->changed = true;
  }
}

gboolean
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
