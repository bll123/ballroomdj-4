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
#include "dance.h"
#include "ilist.h"
#include "log.h"
#include "ui.h"

bool
confuiTableAdd (void *udata)
{
  confuigui_t       *gui = udata;
  GtkWidget         *tree = NULL;
  GtkTreeModel      *model = NULL;
  GtkTreeIter       iter;
  GtkTreeIter       niter;
  GtkTreeIter       *titer;
  GtkTreePath       *path;
  int               count;
  int               flags;

  logProcBegin (LOG_PROC, "confuiTableAdd");

  if (gui->tablecurr >= CONFUI_ID_TABLE_MAX) {
    logProcEnd (LOG_PROC, "confuiTableAdd", "non-table");
    return UICB_STOP;
  }

  tree = gui->tables [gui->tablecurr].tree;
  if (tree == NULL) {
    logProcEnd (LOG_PROC, "confuiTableAdd", "no-tree");
    return UICB_STOP;
  }

  flags = gui->tables [gui->tablecurr].flags;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  count = gtk_tree_selection_count_selected_rows (
      gui->tables [gui->tablecurr].sel);
  titer = NULL;
  if (count == 1) {
    int   valid;

    valid = gtk_tree_selection_get_selected (
        gui->tables [gui->tablecurr].sel, &model, &iter);
    if (valid) {
      titer = &iter;
    }
  }

  if (titer != NULL) {
    GtkTreePath       *path;
    char              *pathstr;
    int               idx;

    path = gtk_tree_model_get_path (model, titer);
    pathstr = gtk_tree_path_to_string (path);
    sscanf (pathstr, "%d", &idx);
    free (pathstr);
    gtk_tree_path_free (path);
    if (idx == 0 &&
        (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
      gtk_tree_model_iter_next (model, &iter);
    }
  }

  if (titer == NULL) {
    gtk_list_store_append (GTK_LIST_STORE (model), &niter);
  } else {
    gtk_list_store_insert_before (GTK_LIST_STORE (model), &niter, &iter);
  }

  switch (gui->tablecurr) {
    case CONFUI_ID_DANCE: {
      dance_t     *dances;
      ilistidx_t  dkey;

      dances = bdjvarsdfGet (BDJVDF_DANCES);
      /* CONTEXT: configuration: dance name that is set when adding a new dance */
      dkey = danceAdd (dances, _("New Dance"));
      /* CONTEXT: configuration: dance name that is set when adding a new dance */
      confuiDanceSet (GTK_LIST_STORE (model), &niter, _("New Dance"), dkey);
      break;
    }

    case CONFUI_ID_GENRES: {
      /* CONTEXT: configuration: genre name that is set when adding a new genre */
      confuiGenreSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Genre"), 0);
      break;
    }

    case CONFUI_ID_RATINGS: {
      /* CONTEXT: configuration: rating name that is set when adding a new rating */
      confuiRatingSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Rating"), 0);
      break;
    }

    case CONFUI_ID_LEVELS: {
      /* CONTEXT: configuration: level name that is set when adding a new level */
      confuiLevelSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Level"), 0, 0);
      break;
    }

    case CONFUI_ID_STATUS: {
      /* CONTEXT: configuration: status name that is set when adding a new status */
      confuiStatusSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Status"), 0);
      break;
    }

    default: {
      break;
    }
  }

  path = gtk_tree_model_get_path (model, &niter);
  gtk_tree_selection_select_path (
      gui->tables [gui->tablecurr].sel, path);
  if (gui->tablecurr == CONFUI_ID_DANCE) {
    confuiDanceSelect (GTK_TREE_VIEW (tree), path, NULL, gui);
  }
  gtk_tree_path_free (path);

  gui->tables [gui->tablecurr].changed = true;
  gui->tables [gui->tablecurr].currcount += 1;
  logProcEnd (LOG_PROC, "confuiTableAdd", "");
  return UICB_CONT;
}

