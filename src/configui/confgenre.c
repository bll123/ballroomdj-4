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
#include "genre.h"
#include "ilist.h"
#include "log.h"
#include "tagdef.h"
#include "ui.h"

static int  confuiGenreListCreate (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer udata);
static void confuiGenreSave (confuigui_t *gui);

void
confuiBuildUIEditGenres (confuigui_t *gui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIEditGenres");
  uiCreateVertBox (&vbox);

  /* edit genres */
  confuiMakeNotebookTab (&vbox, gui,
      /* CONTEXT: configuration: edit genres table */
      _("Edit Genres"), CONFUI_ID_GENRES);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: genres: information on how to edit a genre entry */
  uiCreateLabel (&uiwidget, _("Double click on a field to edit."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (gui, &hbox, CONFUI_ID_GENRES, CONFUI_TABLE_NONE);
  gui->tables [CONFUI_ID_GENRES].togglecol = CONFUI_GENRE_COL_CLASSICAL;
  gui->tables [CONFUI_ID_GENRES].listcreatefunc = confuiGenreListCreate;
  gui->tables [CONFUI_ID_GENRES].savefunc = confuiGenreSave;
  confuiCreateGenreTable (gui);
  logProcEnd (LOG_PROC, "confuiBuildUIEditGenres", "");
}

void
confuiCreateGenreTable (confuigui_t *gui)
{
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  ilistidx_t        iteridx;
  ilistidx_t        key;
  genre_t           *genres;
  uitree_t          *uitree;
  UIWidget          *uitreewidgetp;

  logProcBegin (LOG_PROC, "confuiCreateGenreTable");

  genres = bdjvarsdfGet (BDJVDF_GENRES);

  uitree = gui->tables [CONFUI_ID_GENRES].uitree;
  uitreewidgetp = uiTreeViewGetUIWidget (uitree);

  uiTreeViewCreateValueStore (uitree, CONFUI_GENRE_COL_MAX,
      TREE_TYPE_NUM, TREE_TYPE_STRING, TREE_TYPE_BOOLEAN, TREE_TYPE_STRING,
      TREE_TYPE_END);

  genreStartIterator (genres, &iteridx);

  while ((key = genreIterate (genres, &iteridx)) >= 0) {
    char    *genredisp;
    long    clflag;

    genredisp = genreGetGenre (genres, key);
    clflag = genreGetClassicalFlag (genres, key);

    uiTreeViewValueAppend (uitree);
    confuiGenreSet (uitree, TRUE, genredisp, clflag);
    gui->tables [CONFUI_ID_GENRES].currcount += 1;
  }

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "uicolumn",
      GUINT_TO_POINTER (CONFUI_GENRE_COL_GENRE));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_GENRE_COL_GENRE,
      "editable", CONFUI_GENRE_COL_EDITABLE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_title (column, tagdefs [TAG_GENRE].displayname);
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditText), gui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (G_OBJECT(renderer), "toggled",
      G_CALLBACK (confuiTableToggle), gui);
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "active", CONFUI_GENRE_COL_CLASSICAL,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: genre: title of the classical setting column */
  gtk_tree_view_column_set_title (column, _("Classical?"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_GENRE_COL_SB_PAD,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

  logProcEnd (LOG_PROC, "confuiCreateGenreTable", "");
}


static gboolean
confuiGenreListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata)
{
  confuigui_t *gui = udata;
  char        *genredisp;
  gboolean    clflag;

  logProcBegin (LOG_PROC, "confuiGenreListCreate");
  gtk_tree_model_get (model, iter,
      CONFUI_GENRE_COL_GENRE, &genredisp,
      CONFUI_GENRE_COL_CLASSICAL, &clflag,
      -1);
  ilistSetStr (gui->tables [CONFUI_ID_GENRES].savelist,
      gui->tables [CONFUI_ID_GENRES].saveidx, GENRE_GENRE, genredisp);
  ilistSetNum (gui->tables [CONFUI_ID_GENRES].savelist,
      gui->tables [CONFUI_ID_GENRES].saveidx, GENRE_CLASSICAL_FLAG, clflag);
  gui->tables [CONFUI_ID_GENRES].saveidx += 1;
  logProcEnd (LOG_PROC, "confuiGenreListCreate", "");
  return FALSE;
}

static void
confuiGenreSave (confuigui_t *gui)
{
  genre_t    *genres;

  logProcBegin (LOG_PROC, "confuiGenreSave");
  genres = bdjvarsdfGet (BDJVDF_GENRES);
  genreSave (genres, gui->tables [CONFUI_ID_GENRES].savelist);
  ilistFree (gui->tables [CONFUI_ID_GENRES].savelist);
  logProcEnd (LOG_PROC, "confuiGenreSave", "");
}

