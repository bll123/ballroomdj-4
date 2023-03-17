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
#include <stdarg.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjvarsdf.h"
#include "configui.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "status.h"
#include "tagdef.h"
#include "ui.h"

static int  confuiStatusListCreate (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static void confuiStatusSave (confuigui_t *gui);

void
confuiBuildUIEditStatus (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIEditStatus");
  uiCreateVertBox (&vbox);

  /* edit status */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: edit status table */
      _("Edit Status"), CONFUI_ID_STATUS);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: status: information on how to edit a status entry */
  uiCreateLabel (&uiwidget, _("Double click on a field to edit."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (gui, &hbox, CONFUI_ID_STATUS,
      CONFUI_TABLE_KEEP_FIRST | CONFUI_TABLE_KEEP_LAST);
  gui->tables [CONFUI_ID_STATUS].togglecol = CONFUI_STATUS_COL_PLAY_FLAG;
  gui->tables [CONFUI_ID_STATUS].listcreatefunc = confuiStatusListCreate;
  gui->tables [CONFUI_ID_STATUS].savefunc = confuiStatusSave;
  confuiCreateStatusTable (gui);
  logProcEnd (LOG_PROC, "confuiBuildUIEditStatus", "");
}

void
confuiCreateStatusTable (confuigui_t *gui)
{
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  ilistidx_t        iteridx;
  ilistidx_t        key;
  status_t          *status;
  uitree_t          *uitree;
  UIWidget          *uitreewidgetp;
  int               editable;

  logProcBegin (LOG_PROC, "confuiCreateStatusTable");

  status = bdjvarsdfGet (BDJVDF_STATUS);

  uitree = gui->tables [CONFUI_ID_STATUS].uitree;
  uitreewidgetp = uiTreeViewGetUIWidget (uitree);

  uiTreeViewCreateValueStore (uitree, CONFUI_STATUS_COL_MAX,
      TREE_TYPE_NUM, TREE_TYPE_STRING, TREE_TYPE_BOOLEAN, TREE_TYPE_END);

  statusStartIterator (status, &iteridx);

  editable = FALSE;
  while ((key = statusIterate (status, &iteridx)) >= 0) {
    char    *statusdisp;
    long playflag;

    statusdisp = statusGetStatus (status, key);
    playflag = statusGetPlayFlag (status, key);

    if (key == statusGetCount (status) - 1) {
      editable = FALSE;
    }

    uiTreeViewValueAppend (uitree);
//    gtk_list_store_append (store, &iter);
    confuiStatusSet (uitree, editable, statusdisp, playflag);
//    confuiStatusSet (store, &iter, editable, statusdisp, playflag);
    /* all cells other than the very first (Unrated) are editable */
    editable = TRUE;
    gui->tables [CONFUI_ID_STATUS].currcount += 1;
  }

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "uicolumn",
      GUINT_TO_POINTER (CONFUI_STATUS_COL_STATUS));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_STATUS_COL_STATUS,
      "editable", CONFUI_STATUS_COL_EDITABLE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_title (column, tagdefs [TAG_STATUS].displayname);
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditText), gui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (G_OBJECT(renderer), "toggled",
      G_CALLBACK (confuiTableToggle), gui);
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "active", CONFUI_STATUS_COL_PLAY_FLAG,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: status: title of the "playable" column */
  gtk_tree_view_column_set_title (column, _("Play?"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

//  gtk_tree_view_set_model (GTK_TREE_VIEW (uitreewidgetp->widget), GTK_TREE_MODEL (store));
//  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateStatusTable", "");
}

static gboolean
confuiStatusListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata)
{
  confuigui_t *gui = udata;
  char        *statusdisp;
  gboolean    playflag;

  logProcBegin (LOG_PROC, "confuiStatusListCreate");
  gtk_tree_model_get (model, iter,
      CONFUI_STATUS_COL_STATUS, &statusdisp,
      CONFUI_STATUS_COL_PLAY_FLAG, &playflag,
      -1);
  ilistSetStr (gui->tables [CONFUI_ID_STATUS].savelist,
      gui->tables [CONFUI_ID_STATUS].saveidx, STATUS_STATUS, statusdisp);
  ilistSetNum (gui->tables [CONFUI_ID_STATUS].savelist,
      gui->tables [CONFUI_ID_STATUS].saveidx, STATUS_PLAY_FLAG, playflag);
  mdfree (statusdisp);
  gui->tables [CONFUI_ID_STATUS].saveidx += 1;
  logProcEnd (LOG_PROC, "confuiStatusListCreate", "");
  return FALSE;
}

static void
confuiStatusSave (confuigui_t *gui)
{
  status_t    *status;

  logProcBegin (LOG_PROC, "confuiStatusSave");
  status = bdjvarsdfGet (BDJVDF_STATUS);
  statusSave (status, gui->tables [CONFUI_ID_STATUS].savelist);
  ilistFree (gui->tables [CONFUI_ID_STATUS].savelist);
  logProcEnd (LOG_PROC, "confuiStatusSave", "");
}

