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
#include "log.h"
#include "rating.h"
#include "ui.h"

static int    confuiRatingListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata);
static void   confuiRatingSave (confuigui_t *gui);

void
confuiBuildUIEditRatings (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIEditRatings");
  uiCreateVertBox (&vbox);

  /* edit ratings */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: edit the dance ratings table */
      _("Edit Ratings"), CONFUI_ID_RATINGS);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: dance ratings: information on how to order the ratings */
  uiCreateLabel (&uiwidget, _("Order from the lowest rating to the highest rating."));
  uiBoxPackStart (&vbox, &uiwidget);

  /* CONTEXT: configuration: dance ratings: information on how to edit a rating entry */
  uiCreateLabel (&uiwidget, _("Double click on a field to edit."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (gui, &hbox, CONFUI_ID_RATINGS, CONFUI_TABLE_KEEP_FIRST);
  gui->tables [CONFUI_ID_RATINGS].listcreatefunc = confuiRatingListCreate;
  gui->tables [CONFUI_ID_RATINGS].savefunc = confuiRatingSave;
  confuiCreateRatingTable (gui);
  logProcEnd (LOG_PROC, "confuiBuildUIEditRatings", "");
}

void
confuiCreateRatingTable (confuigui_t *gui)
{
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  ilistidx_t        iteridx;
  ilistidx_t        key;
  rating_t          *ratings;
  UIWidget          *uitree;
  int               editable;

  logProcBegin (LOG_PROC, "confuiCreateRatingTable");

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);

  store = gtk_list_store_new (CONFUI_RATING_COL_MAX,
      G_TYPE_LONG, G_TYPE_LONG, G_TYPE_STRING,
      G_TYPE_LONG, G_TYPE_OBJECT, G_TYPE_LONG);
  assert (store != NULL);

  ratingStartIterator (ratings, &iteridx);

  editable = FALSE;
  while ((key = ratingIterate (ratings, &iteridx)) >= 0) {
    char    *ratingdisp;
    long weight;

    ratingdisp = ratingGetRating (ratings, key);
    weight = ratingGetWeight (ratings, key);

    gtk_list_store_append (store, &iter);
    confuiRatingSet (store, &iter, editable, ratingdisp, weight);
    /* all cells other than the very first (Unrated) are editable */
    editable = TRUE;
    gui->tables [CONFUI_ID_RATINGS].currcount += 1;
  }

  uitree = &gui->tables [CONFUI_ID_RATINGS].uitree;

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_RATING_COL_RATING));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_RATING_COL_RATING,
      "editable", CONFUI_RATING_COL_R_EDITABLE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: rating: title of the rating name column */
  gtk_tree_view_column_set_title (column, _("Rating"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditText), gui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitree->widget), column);

  renderer = gtk_cell_renderer_spin_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_RATING_COL_WEIGHT));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_RATING_COL_WEIGHT,
      "editable", CONFUI_RATING_COL_W_EDITABLE,
      "adjustment", CONFUI_RATING_COL_ADJUST,
      "digits", CONFUI_RATING_COL_DIGITS,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: rating: title of the weight column */
  gtk_tree_view_column_set_title (column, _("Weight"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditSpinbox), gui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitree->widget), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (uitree->widget), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateRatingTable", "");
}

static gboolean
confuiRatingListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata)
{
  confuigui_t *gui = udata;
  char        *ratingdisp;
  glong       weight;

  logProcBegin (LOG_PROC, "confuiRatingListCreate");
  gtk_tree_model_get (model, iter,
      CONFUI_RATING_COL_RATING, &ratingdisp,
      CONFUI_RATING_COL_WEIGHT, &weight,
      -1);
  ilistSetStr (gui->tables [CONFUI_ID_RATINGS].savelist,
      gui->tables [CONFUI_ID_RATINGS].saveidx, RATING_RATING, ratingdisp);
  ilistSetNum (gui->tables [CONFUI_ID_RATINGS].savelist,
      gui->tables [CONFUI_ID_RATINGS].saveidx, RATING_WEIGHT, weight);
  free (ratingdisp);
  gui->tables [CONFUI_ID_RATINGS].saveidx += 1;
  logProcEnd (LOG_PROC, "confuiRatingListCreate", "");
  return FALSE;
}

static void
confuiRatingSave (confuigui_t *gui)
{
  rating_t    *ratings;

  logProcBegin (LOG_PROC, "confuiRatingSave");
  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  ratingSave (ratings, gui->tables [CONFUI_ID_RATINGS].savelist);
  ilistFree (gui->tables [CONFUI_ID_RATINGS].savelist);
  logProcEnd (LOG_PROC, "confuiRatingSave", "");
}

