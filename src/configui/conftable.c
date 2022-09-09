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
#include <stdarg.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "configui.h"
#include "dance.h"
#include "ilist.h"
#include "log.h"
#include "ui.h"

/* table editing */
static bool   confuiTableMoveUp (void *udata);
static bool   confuiTableMoveDown (void *udata);
static void   confuiTableMove (confuigui_t *gui, int dir);
static bool   confuiTableRemove (void *udata);
static void   confuiTableSetDefaultSelection (confuigui_t *gui, GtkWidget *tree, GtkTreeSelection *sel);
static void   confuiTableEdit (confuigui_t *gui, GtkCellRendererText* r,
    const gchar* path, const gchar* ntext, int type);

void
confuiMakeItemTable (confuigui_t *gui, UIWidget *boxp, confuiident_t id,
    int flags)
{
  UIWidget    mhbox;
  UIWidget    bvbox;
  UIWidget    uiwidget;
  GtkWidget   *tree;

  logProcBegin (LOG_PROC, "confuiMakeItemTable");
  uiCreateHorizBox (&mhbox);
  uiWidgetSetMarginTop (&mhbox, uiBaseMarginSz * 2);
  uiBoxPackStart (boxp, &mhbox);

  uiCreateScrolledWindow (&uiwidget, 300);
  uiWidgetExpandVert (&uiwidget);
  uiBoxPackStartExpand (&mhbox, &uiwidget);

  tree = uiCreateTreeView ();
  gui->tables [id].tree = tree;
  gui->tables [id].sel =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gui->tables [id].flags = flags;
  uiWidgetSetMarginStartW (tree, uiBaseMarginSz * 8);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), TRUE);
  uiBoxPackInWindowUW (&uiwidget, tree);

  uiCreateVertBox (&bvbox);
  uiWidgetSetAllMargins (&bvbox, uiBaseMarginSz * 4);
  uiWidgetSetMarginTop (&bvbox, uiBaseMarginSz * 32);
  uiWidgetAlignVertStart (&bvbox);
  uiBoxPackStart (&mhbox, &bvbox);

  if ((flags & CONFUI_TABLE_NO_UP_DOWN) != CONFUI_TABLE_NO_UP_DOWN) {
    uiutilsUICallbackInit (
        &gui->tables [id].callback [CONFUI_TABLE_CB_UP],
        confuiTableMoveUp, gui, NULL);
    uiCreateButton (&uiwidget,
        &gui->tables [id].callback [CONFUI_TABLE_CB_UP],
        /* CONTEXT: configuration: table edit: button: move selection up */
        _("Move Up"), "button_up");
    uiBoxPackStart (&bvbox, &uiwidget);

    uiutilsUICallbackInit (
        &gui->tables [id].callback [CONFUI_TABLE_CB_DOWN],
        confuiTableMoveDown, gui, NULL);
    uiCreateButton (&uiwidget,
        &gui->tables [id].callback [CONFUI_TABLE_CB_DOWN],
        /* CONTEXT: configuration: table edit: button: move selection down */
        _("Move Down"), "button_down");
    uiBoxPackStart (&bvbox, &uiwidget);
  }

  uiutilsUICallbackInit (
      &gui->tables [id].callback [CONFUI_TABLE_CB_REMOVE],
      confuiTableRemove, gui, NULL);
  uiCreateButton (&uiwidget,
      &gui->tables [id].callback [CONFUI_TABLE_CB_REMOVE],
      /* CONTEXT: configuration: table edit: button: delete selection */
      _("Delete"), "button_remove");
  uiBoxPackStart (&bvbox, &uiwidget);

  uiutilsUICallbackInit (
      &gui->tables [id].callback [CONFUI_TABLE_CB_ADD],
      confuiTableAdd, gui, NULL);
  uiCreateButton (&uiwidget,
      &gui->tables [id].callback [CONFUI_TABLE_CB_ADD],
      /* CONTEXT: configuration: table edit: button: add new selection */
      _("Add New"), "button_add");
  uiBoxPackStart (&bvbox, &uiwidget);

  logProcEnd (LOG_PROC, "confuiMakeItemTable", "");
}

void
confuiTableSave (confuigui_t *gui, confuiident_t id)
{
  GtkWidget     *tree;
  GtkTreeModel  *model;
  savefunc_t    savefunc;
  char          tbuff [40];

  logProcBegin (LOG_PROC, "confuiTableSave");
  if (gui->tables [id].changed == false) {
    logProcEnd (LOG_PROC, "confuiTableSave", "not-changed");
    return;
  }
  if (gui->tables [id].savefunc == NULL) {
    logProcEnd (LOG_PROC, "confuiTableSave", "no-savefunc");
    return;
  }

  tree = gui->tables [id].tree;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  if (gui->tables [id].listcreatefunc != NULL) {
    snprintf (tbuff, sizeof (tbuff), "cu-table-save-%d", id);
    gui->tables [id].savelist = ilistAlloc (tbuff, LIST_ORDERED);
    gui->tables [id].saveidx = 0;
    gtk_tree_model_foreach (model, gui->tables [id].listcreatefunc, gui);
  }
  savefunc = gui->tables [id].savefunc;
  savefunc (gui);
  logProcEnd (LOG_PROC, "confuiTableSave", "");
}

void
confuiTableEditText (GtkCellRendererText* r, const gchar* path,
    const gchar* ntext, gpointer udata)
{
  confuigui_t   *gui = udata;

  logProcBegin (LOG_PROC, "confuiTableEditText");
  confuiTableEdit (gui, r, path, ntext, CONFUI_TABLE_TEXT);
  logProcEnd (LOG_PROC, "confuiTableEditText", "");
}

void
confuiTableToggle (GtkCellRendererToggle *renderer, gchar *spath, gpointer udata)
{
  confuigui_t   *gui = udata;
  gboolean      val;
  GtkTreeIter   iter;
  GtkTreePath   *path;
  GtkTreeModel  *model;
  int           col;

  logProcBegin (LOG_PROC, "confuiTableToggle");
  model = gtk_tree_view_get_model (
      GTK_TREE_VIEW (gui->tables [gui->tablecurr].tree));
  path = gtk_tree_path_new_from_string (spath);
  if (path != NULL) {
    if (gtk_tree_model_get_iter (model, &iter, path) == FALSE) {
      logProcEnd (LOG_PROC, "confuiTableToggle", "no model/iter");
      return;
    }
    col = gui->tables [gui->tablecurr].togglecol;
    gtk_tree_model_get (model, &iter, col, &val, -1);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, !val, -1);
    gtk_tree_path_free (path);
  }
  gui->tables [gui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiTableToggle", "");
}

void
confuiTableEditSpinbox (GtkCellRendererText* r, const gchar* path,
    const gchar* ntext, gpointer udata)
{
  confuigui_t   *gui = udata;

  logProcBegin (LOG_PROC, "confuiTableEditSpinbox");
  confuiTableEdit (gui, r, path, ntext, CONFUI_TABLE_NUM);
  logProcEnd (LOG_PROC, "confuiTableEditSpinbox", "");
}

void
confuiTableRadioToggle (GtkCellRendererToggle *renderer, gchar *path, gpointer udata)
{
  confuigui_t   *gui = udata;
  GtkTreeIter   iter;
  GtkTreeModel  *model;
  GtkListStore  *store;
  char          tmp [40];
  int           col;
  int           row;

  logProcBegin (LOG_PROC, "confuiTableRadioToggle");
  model = gtk_tree_view_get_model (
      GTK_TREE_VIEW (gui->tables [gui->tablecurr].tree));

  store = GTK_LIST_STORE (model);
  col = gui->tables [gui->tablecurr].togglecol;
  row = gui->tables [gui->tablecurr].radiorow;
  snprintf (tmp, sizeof (tmp), "%d", row);

  if (gtk_tree_model_get_iter_from_string (model, &iter, path)) {
    gtk_list_store_set (store, &iter, col, 1, -1);
  }

  if (gtk_tree_model_get_iter_from_string (model, &iter, tmp)) {
    gtk_list_store_set (store, &iter, col, 0, -1);
  }

  sscanf (path, "%d", &row);
  gui->tables [gui->tablecurr].radiorow = row;
  gui->tables [gui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiTableRadioToggle", "");
}

bool
confuiSwitchTable (void *udata, long pagenum)
{
  confuigui_t       *gui = udata;
  GtkWidget         *tree;
  confuiident_t     newid;

  logProcBegin (LOG_PROC, "confuiSwitchTable");
  if ((newid = (confuiident_t) uiutilsNotebookIDGet (gui->nbtabid, pagenum)) < 0) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "bad-pagenum");
    return UICB_STOP;
  }

  if (gui->tablecurr == newid) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "same-id");
    return UICB_CONT;
  }

  confuiSetStatusMsg (gui, "");

  gui->tablecurr = (confuiident_t) uiutilsNotebookIDGet (
      gui->nbtabid, pagenum);

  if (gui->tablecurr == CONFUI_ID_MOBILE_MQ) {
    confuiUpdateMobmqQrcode (gui);
  }
  if (gui->tablecurr == CONFUI_ID_REM_CONTROL) {
    confuiUpdateRemctrlQrcode (gui);
  }
  if (gui->tablecurr == CONFUI_ID_ORGANIZATION) {
    confuiUpdateOrgExamples (gui, bdjoptGetStr (OPT_G_AO_PATHFMT));
  }
  if (gui->tablecurr == CONFUI_ID_DISP_SEL_LIST) {
    /* be sure to create the listing first */
    confuiCreateTagListingDisp (gui);
    confuiCreateTagSelectedDisp (gui);
  }

  if (gui->tablecurr >= CONFUI_ID_TABLE_MAX) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "non-table");
    return UICB_CONT;
  }

  tree = gui->tables [gui->tablecurr].tree;
  if (tree == NULL) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "no-tree");
    return UICB_CONT;
  }

  confuiTableSetDefaultSelection (gui, tree,
      gui->tables [gui->tablecurr].sel);

  logProcEnd (LOG_PROC, "confuiSwitchTable", "");
  return UICB_CONT;
}

/* internal routines */

/* table editing */

static bool
confuiTableMoveUp (void *udata)
{
  logProcBegin (LOG_PROC, "confuiTableMoveUp");
  confuiTableMove (udata, CONFUI_MOVE_PREV);
  logProcEnd (LOG_PROC, "confuiTableMoveUp", "");
  return UICB_CONT;
}

static bool
confuiTableMoveDown (void *udata)
{
  logProcBegin (LOG_PROC, "confuiTableMoveDown");
  confuiTableMove (udata, CONFUI_MOVE_NEXT);
  logProcEnd (LOG_PROC, "confuiTableMoveDown", "");
  return UICB_CONT;
}

static void
confuiTableMove (confuigui_t *gui, int dir)
{
  GtkWidget         *tree;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GtkTreeIter       citer;
  GtkTreePath       *path = NULL;
  char              *pathstr = NULL;
  int               count;
  gboolean          valid;
  int               idx;
  int               flags;

  logProcBegin (LOG_PROC, "confuiTableMove");
  tree = gui->tables [gui->tablecurr].tree;
  flags = gui->tables [gui->tablecurr].flags;

  if (tree == NULL) {
    return;
  }

  count = gtk_tree_selection_count_selected_rows (
      gui->tables [gui->tablecurr].sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiTableMove", "no-selection");
    return;
  }

  gtk_tree_selection_get_selected (
      gui->tables [gui->tablecurr].sel, &model, &iter);

  path = gtk_tree_model_get_path (model, &iter);
  if (path == NULL) {
    return;
  }

  pathstr = gtk_tree_path_to_string (path);
  sscanf (pathstr, "%d", &idx);
  free (pathstr);
  gtk_tree_path_free (path);

  if (idx == 1 &&
      dir == CONFUI_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-prev-keep-first");
    return;
  }
  if (idx == gui->tables [gui->tablecurr].currcount - 1 &&
      dir == CONFUI_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-prev-keep-last");
    return;
  }
  if (idx == gui->tables [gui->tablecurr].currcount - 2 &&
      dir == CONFUI_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-next-keep-last");
    return;
  }
  if (idx == 0 &&
      dir == CONFUI_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-next-keep-first");
    return;
  }

  memcpy (&citer, &iter, sizeof (iter));
  if (dir == CONFUI_MOVE_PREV) {
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
  gui->tables [gui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiTableMove", "");
}

static bool
confuiTableRemove (void *udata)
{
  confuigui_t       *gui = udata;
  GtkWidget         *tree;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GtkTreePath       *path;
  char              *pathstr;
  int               idx;
  int               count;
  int               flags;

  logProcBegin (LOG_PROC, "confuiTableRemove");
  tree = gui->tables [gui->tablecurr].tree;

  if (tree == NULL) {
    return UICB_STOP;
  }

  flags = gui->tables [gui->tablecurr].flags;
  count = uiTreeViewGetSelection (tree, &model, &iter);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "no-selection");
    return UICB_STOP;
  }

  path = gtk_tree_model_get_path (model, &iter);
  pathstr = gtk_tree_path_to_string (path);
  sscanf (pathstr, "%d", &idx);
  free (pathstr);
  gtk_tree_path_free (path);
  if (idx == 0 &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "keep-first");
    return UICB_CONT;
  }
  if (idx == gui->tables [gui->tablecurr].currcount - 1 &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "keep-last");
    return UICB_CONT;
  }

  if (gui->tablecurr == CONFUI_ID_DANCE) {
    long          idx;
    dance_t       *dances;
    GtkWidget     *tree;
    GtkTreeModel  *model;
    GtkTreeIter   iter;

    tree = gui->tables [gui->tablecurr].tree;
    uiTreeViewGetSelection (tree, &model, &iter);
    gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &idx, -1);
    dances = bdjvarsdfGet (BDJVDF_DANCES);
    danceDelete (dances, idx);
  }

  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  gui->tables [gui->tablecurr].changed = true;
  gui->tables [gui->tablecurr].currcount -= 1;
  logProcEnd (LOG_PROC, "confuiTableRemove", "");

  if (gui->tablecurr == CONFUI_ID_DANCE) {
    GtkWidget   *tree;
    GtkTreePath *path;
    GtkTreeIter iter;

    tree = gui->tables [gui->tablecurr].tree;
    uiTreeViewGetSelection (tree, &model, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    confuiDanceSelect (GTK_TREE_VIEW (tree), path, NULL, gui);
  }

  return UICB_CONT;
}

static void
confuiTableSetDefaultSelection (confuigui_t *gui, GtkWidget *tree,
    GtkTreeSelection *sel)
{
  int               count;
  GtkTreeIter       iter;
  GtkTreeModel      *model;


  count = uiTreeViewGetSelection (tree, &model, &iter);
  if (count != 1) {
    GtkTreePath   *path;

    path = gtk_tree_path_new_from_string ("0");
    if (path != NULL) {
      gtk_tree_selection_select_path (sel, path);
    }
    if (gui->tablecurr == CONFUI_ID_DANCE) {
      confuiDanceSelect (GTK_TREE_VIEW (tree), path, NULL, gui);
    }
    if (path != NULL) {
      gtk_tree_path_free (path);
    }
  }
}

static void
confuiTableEdit (confuigui_t *gui, GtkCellRendererText* r,
    const gchar* path, const gchar* ntext, int type)
{
  GtkWidget     *tree;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  int           col;

  logProcBegin (LOG_PROC, "confuiTableEdit");
  tree = gui->tables [gui->tablecurr].tree;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  gtk_tree_model_get_iter_from_string (model, &iter, path);
  col = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (r), "confuicolumn"));
  if (type == CONFUI_TABLE_TEXT) {
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, ntext, -1);
  }
  if (type == CONFUI_TABLE_NUM) {
    long val = atol (ntext);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, val, -1);
  }
  gui->tables [gui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiTableEdit", "");
}

