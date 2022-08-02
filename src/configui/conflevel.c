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
#include "bdjvarsdf.h"
#include "configui.h"
#include "ilist.h"
#include "level.h"
#include "log.h"
#include "ui.h"

static int  confuiLevelListCreate (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static void confuiLevelSave (confuigui_t *gui);

void
confuiBuildUIEditLevels (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIEditLevels");
  uiCreateVertBox (&vbox);

  /* edit levels */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: edit dance levels table */
      _("Edit Levels"), CONFUI_ID_LEVELS);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: dance levels: instructions */
  uiCreateLabel (&uiwidget, _("Order from easiest to most advanced."));
  uiBoxPackStart (&vbox, &uiwidget);

  /* CONTEXT: configuration: dance levels: information on how to edit a level entry */
  uiCreateLabel (&uiwidget, _("Double click on a field to edit."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (gui, &hbox, CONFUI_ID_LEVELS, CONFUI_TABLE_NONE);
  gui->tables [CONFUI_ID_LEVELS].togglecol = CONFUI_LEVEL_COL_DEFAULT;
  gui->tables [CONFUI_ID_LEVELS].listcreatefunc = confuiLevelListCreate;
  gui->tables [CONFUI_ID_LEVELS].savefunc = confuiLevelSave;
  confuiCreateLevelTable (gui);
  logProcEnd (LOG_PROC, "confuiBuildUIEditLevels", "");
}

void
confuiCreateLevelTable (confuigui_t *gui)
{
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  ilistidx_t        iteridx;
  ilistidx_t        key;
  level_t           *levels;
  GtkWidget         *tree;

  logProcBegin (LOG_PROC, "confuiCreateLevelTable");

  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  store = gtk_list_store_new (CONFUI_LEVEL_COL_MAX,
      G_TYPE_LONG, G_TYPE_STRING, G_TYPE_LONG, G_TYPE_BOOLEAN,
      G_TYPE_OBJECT, G_TYPE_LONG);
  assert (store != NULL);

  levelStartIterator (levels, &iteridx);

  while ((key = levelIterate (levels, &iteridx)) >= 0) {
    char    *leveldisp;
    long weight;
    long def;

    leveldisp = levelGetLevel (levels, key);
    weight = levelGetWeight (levels, key);
    def = levelGetDefault (levels, key);
    if (def) {
      gui->tables [CONFUI_ID_LEVELS].radiorow = key;
    }

    gtk_list_store_append (store, &iter);
    confuiLevelSet (store, &iter, TRUE, leveldisp, weight, def);
    /* all cells other than the very first (Unrated) are editable */
    gui->tables [CONFUI_ID_LEVELS].currcount += 1;
  }

  tree = gui->tables [CONFUI_ID_LEVELS].tree;

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_LEVEL_COL_LEVEL));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_LEVEL_COL_LEVEL,
      "editable", CONFUI_LEVEL_COL_EDITABLE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: level: title of the level name column */
  gtk_tree_view_column_set_title (column, _("Level"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditText), gui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_spin_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_LEVEL_COL_WEIGHT));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_LEVEL_COL_WEIGHT,
      "editable", CONFUI_LEVEL_COL_EDITABLE,
      "adjustment", CONFUI_LEVEL_COL_ADJUST,
      "digits", CONFUI_LEVEL_COL_DIGITS,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: level: title of the weight column */
  gtk_tree_view_column_set_title (column, _("Weight"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditSpinbox), gui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (G_OBJECT(renderer), "toggled",
      G_CALLBACK (confuiTableRadioToggle), gui);
  gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "active", CONFUI_LEVEL_COL_DEFAULT,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: level: title of the default selection column */
  gtk_tree_view_column_set_title (column, _("Default"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateLevelTable", "");
}

static gboolean
confuiLevelListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata)
{
  confuigui_t *gui = udata;
  char        *leveldisp;
  long        weight;
  gboolean    def;

  logProcBegin (LOG_PROC, "confuiLevelListCreate");
  gtk_tree_model_get (model, iter,
      CONFUI_LEVEL_COL_LEVEL, &leveldisp,
      CONFUI_LEVEL_COL_WEIGHT, &weight,
      CONFUI_LEVEL_COL_DEFAULT, &def,
      -1);
  ilistSetStr (gui->tables [CONFUI_ID_LEVELS].savelist,
      gui->tables [CONFUI_ID_LEVELS].saveidx, LEVEL_LEVEL, leveldisp);
  ilistSetNum (gui->tables [CONFUI_ID_LEVELS].savelist,
      gui->tables [CONFUI_ID_LEVELS].saveidx, LEVEL_WEIGHT, weight);
  ilistSetNum (gui->tables [CONFUI_ID_LEVELS].savelist,
      gui->tables [CONFUI_ID_LEVELS].saveidx, LEVEL_DEFAULT_FLAG, def);
  free (leveldisp);
  gui->tables [CONFUI_ID_LEVELS].saveidx += 1;
  logProcEnd (LOG_PROC, "confuiLevelListCreate", "");
  return FALSE;
}

static void
confuiLevelSave (confuigui_t *gui)
{
  level_t    *levels;

  logProcBegin (LOG_PROC, "confuiLevelSave");
  levels = bdjvarsdfGet (BDJVDF_LEVELS);
  levelSave (levels, gui->tables [CONFUI_ID_LEVELS].savelist);
  ilistFree (gui->tables [CONFUI_ID_LEVELS].savelist);
  logProcEnd (LOG_PROC, "confuiLevelSave", "");
}

