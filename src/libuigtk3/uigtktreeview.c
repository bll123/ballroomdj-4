/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "tagdef.h"
#include "ui.h"

static GType * uiAppendType (GType *types, int ncol, int type);
static void uiTreeViewEditedSignalHandler (GtkCellRendererText* r, const gchar* path, const gchar* ntext, gpointer udata);

void
uiCreateTreeView (UIWidget *uiwidget)
{
  GtkWidget         *tree;
  GtkTreeSelection  *sel;

  tree = gtk_tree_view_new ();
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (tree), FALSE);
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (tree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_halign (tree, GTK_ALIGN_START);
  gtk_widget_set_hexpand (tree, FALSE);
  gtk_widget_set_vexpand (tree, FALSE);
  uiwidget->widget = tree;
  uiWidgetSetAllMargins (uiwidget, 2);
}

void
uiTreeViewAddEditableColumn (UIWidget *uitree, int col, int editcol,
    const char *title, UICallback *uicb)
{
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "uicolumn",
      GUINT_TO_POINTER (col));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", col,
      "editable", editcol,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_title (column, title);
  g_signal_connect (renderer, "edited",
      G_CALLBACK (uiTreeViewEditedSignalHandler), uicb);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitree->widget), column);
}

GtkTreeViewColumn *
uiTreeViewAddDisplayColumns (UIWidget *uitree, slist_t *sellist, int col,
    int fontcol, int ellipsizeCol)
{
  slistidx_t  seliteridx;
  int         tagidx;
  GtkCellRenderer       *renderer = NULL;
  GtkTreeViewColumn     *column = NULL;
  GtkTreeViewColumn     *favColumn = NULL;


  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    renderer = gtk_cell_renderer_text_new ();
    if (tagidx == TAG_FAVORITE) {
      /* use the normal UI font here */
      column = gtk_tree_view_column_new_with_attributes ("", renderer,
          "markup", col,
          NULL);
      gtk_cell_renderer_set_alignment (renderer, 0.5, 0.5);
      favColumn = column;
    } else {
      column = gtk_tree_view_column_new_with_attributes ("", renderer,
          "text", col,
          "font", fontcol,
          NULL);
    }
    if (tagdefs [tagidx].alignRight) {
      gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
    }
    if (tagdefs [tagidx].ellipsize) {
      if (tagidx == TAG_TITLE) {
        gtk_tree_view_column_set_min_width (column, 200);
      }
      if (tagidx == TAG_ARTIST) {
        gtk_tree_view_column_set_min_width (column, 100);
      }
      gtk_tree_view_column_add_attribute (column, renderer,
          "ellipsize", ellipsizeCol);
      gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
      gtk_tree_view_column_set_expand (column, TRUE);
    } else {
      gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
    }
    gtk_tree_view_append_column (GTK_TREE_VIEW (uitree->widget), column);
    if (tagidx == TAG_FAVORITE) {
      gtk_tree_view_column_set_title (column, "\xE2\x98\x86");
    } else {
      gtk_tree_view_column_set_title (column, tagdefs [tagidx].displayname);
    }
    col++;
  }

  return favColumn;
}

void
uiTreeViewSetDisplayColumn (GtkTreeModel *model, GtkTreeIter *iter,
    int col, long num, const char *str)
{
  if (str != NULL) {
    gtk_list_store_set (GTK_LIST_STORE (model), iter, col, str, -1);
  } else {
    char    tstr [40];

    *tstr = '\0';
    if (num >= 0) {
      snprintf (tstr, sizeof (tstr), "%ld", num);
    }
    gtk_list_store_set (GTK_LIST_STORE (model), iter, col, tstr, -1);
  }
}


GType *
uiTreeViewAddDisplayType (GType *types, int valtype, int col)
{
  int     type;

  if (valtype == UITREE_TYPE_NUM) {
    /* despite being a numeric type, the display needs a string */
    /* so that empty values can be displayed */
    type = G_TYPE_STRING;
  }
  if (valtype == UITREE_TYPE_STRING) {
    type = G_TYPE_STRING;
  }
  types = uiAppendType (types, col, type);

  return types;
}

int
uiTreeViewGetSelection (UIWidget *uitree, GtkTreeModel **model, GtkTreeIter *iter)
{
  GtkTreeSelection  *sel;
  int               count;

  if (uitree == NULL || uitree->widget == NULL) {
    return 0;
  }

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uitree->widget));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count == 1) {
    /* this only works if the treeview is in single-selection mode */
    gtk_tree_selection_get_selected (sel, model, iter);
  }
  return count;
}

void
uiTreeViewAllowMultiple (UIWidget *uitree)
{
  GtkTreeSelection  *sel;
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uitree->widget));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);
}

void
uiTreeViewEnableHeaders (UIWidget *uitree)
{
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uitree->widget), TRUE);
}

void
uiTreeViewDisableHeaders (UIWidget *uitree)
{
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uitree->widget), FALSE);
}

void
uiTreeViewDarkBackground (UIWidget *uitree)
{
  uiSetCss (uitree->widget,
      "treeview { background-color: shade(@theme_base_color,0.8); } "
      "treeview:selected { background-color: @theme_selected_bg_color; } ");
}

void
uiTreeViewDisableSingleClick (UIWidget *uitree)
{
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (uitree->widget), FALSE);
}

/* internal routines */

static GType *
uiAppendType (GType *types, int ncol, int type)
{
  types = realloc (types, (ncol + 1) * sizeof (GType));
  types [ncol] = type;

  return types;
}

static void
uiTreeViewEditedSignalHandler (GtkCellRendererText* r, const gchar* path,
    const gchar* ntext, gpointer udata)
{
  UICallback  *cb = udata;

  if (cb != NULL) {
    int   col;

    col = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (r), "uicolumn"));
    // lots of arguments to handle...
  }
}
