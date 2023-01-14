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

#include "mdebug.h"
#include "tagdef.h"
#include "ui.h"

static GType * uiAppendType (GType *types, int ncol, int type);
static void uiTreeViewEditedCallback (GtkCellRendererText* r, const gchar* path, const gchar* ntext, gpointer udata);

typedef struct uitree {
  UIWidget    uitree;
  UIWidget    sel;
  UICallback  *editedcb;
} uitree_t;

uitree_t *
uiCreateTreeView (void)
{
  uitree_t          *uitree;
  GtkWidget         *tree;
  GtkTreeSelection  *sel;

  uitree = mdmalloc (sizeof (uitree_t));

  tree = gtk_tree_view_new ();
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (tree), FALSE);
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (tree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_halign (tree, GTK_ALIGN_START);
  gtk_widget_set_hexpand (tree, FALSE);
  gtk_widget_set_vexpand (tree, FALSE);
  uitree->uitree.widget = tree;
  uitree->editedcb = NULL;
  uitree->sel.sel = sel;
  uiWidgetSetAllMargins (&uitree->uitree, 2);
  return uitree;
}

void
uiTreeViewFree (uitree_t *uitree)
{
  if (uitree == NULL) {
    return;
  }

  mdfree (uitree);
}

UIWidget *
uiTreeViewGetUIWidget (uitree_t *uitree)
{
  if (uitree == NULL) {
    return NULL;
  }

  return &uitree->uitree;
}

int
uiTreeViewGetSelection (uitree_t *uitree, GtkTreeModel **model, GtkTreeIter *iter)
{
  GtkTreeSelection  *sel;
  int               count;

  if (uitree == NULL || uitree->uitree.widget == NULL) {
    return 0;
  }

  sel = uitree->sel.sel;
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count == 1) {
    /* this only works if the treeview is in single-selection mode */
    gtk_tree_selection_get_selected (sel, model, iter);
  }
  return count;
}

void
uiTreeViewAllowMultiple (uitree_t *uitree)
{
  GtkTreeSelection  *sel;

  sel = uitree->sel.sel;
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);
}

void
uiTreeViewEnableHeaders (uitree_t *uitree)
{
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uitree->uitree.widget), TRUE);
}

void
uiTreeViewDisableHeaders (uitree_t *uitree)
{
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uitree->uitree.widget), FALSE);
}

void
uiTreeViewDarkBackground (uitree_t *uitree)
{
  uiSetCss (uitree->uitree.widget,
      "treeview { background-color: shade(@theme_base_color,0.8); } "
      "treeview:selected { background-color: @theme_selected_bg_color; } ");
}

void
uiTreeViewDisableSingleClick (uitree_t *uitree)
{
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (uitree->uitree.widget), FALSE);
}

void
uiTreeViewSelectionSetMode (uitree_t *uitree, int mode)
{
  gtk_tree_selection_set_mode (uitree->sel.sel, mode);
}

void
uiTreeViewSelectionSet (uitree_t *uitree, int row)
{
  char        tbuff [40];
  GtkTreePath *path = NULL;

  snprintf (tbuff, sizeof (tbuff), "%d", row);
  path = gtk_tree_path_new_from_string (tbuff);
  mdextalloc (path);
  if (path != NULL) {
    gtk_tree_selection_select_path (uitree->sel.sel, path);
    mdextfree (path);
    gtk_tree_path_free (path);
  }
}

int
uiTreeViewSelectionGetCount (uitree_t *uitree)
{
  int   count = 0;

  if (uitree == NULL) {
    return count;
  }
  if (uitree->sel.sel == NULL) {
    return count;
  }

  count = gtk_tree_selection_count_selected_rows (uitree->sel.sel);
  return count;
}

/* used by configuration */

void
uiTreeViewSetEditedCallback (uitree_t *uitree, UICallback *uicb)
{
  if (uitree == NULL) {
    return;
  }
  uitree->editedcb = uicb;
}

void
uiTreeViewAddEditableColumn (uitree_t *uitree, int col, int editcol,
    const char *title)
{
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;

  if (uitree == NULL) {
    return;
  }
  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "uicolumn", GUINT_TO_POINTER (col));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", col,
      "editable", editcol,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_title (column, title);
  g_signal_connect (renderer, "edited", G_CALLBACK (uiTreeViewEditedCallback), uitree);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitree->uitree.widget), column);
}

/* the display routines are used by song selection and the music queue */
/* to create the display listings */

GtkTreeViewColumn *
uiTreeViewAddDisplayColumns (uitree_t *uitree, slist_t *sellist, int col,
    int fontcol, int ellipsizeCol)
{
  slistidx_t  seliteridx;
  int         tagidx;
  GtkCellRenderer       *renderer = NULL;
  GtkTreeViewColumn     *column = NULL;
  GtkTreeViewColumn     *favColumn = NULL;


  if (uitree == NULL) {
    return NULL;
  }

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
    gtk_tree_view_append_column (GTK_TREE_VIEW (uitree->uitree.widget), column);
    if (tagidx == TAG_FAVORITE) {
      gtk_tree_view_column_set_title (column, "\xE2\x98\x86");
    } else {
      if (tagdefs [tagidx].shortdisplayname != NULL) {
        gtk_tree_view_column_set_title (column, tagdefs [tagidx].shortdisplayname);
      } else {
        gtk_tree_view_column_set_title (column, tagdefs [tagidx].displayname);
      }
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

/* internal routines */

/* used by the listing display routines */
static GType *
uiAppendType (GType *types, int ncol, int type)
{
  types = mdrealloc (types, (ncol + 1) * sizeof (GType));
  types [ncol] = type;

  return types;
}

/* used by the editable column routines */
static void
uiTreeViewEditedCallback (GtkCellRendererText* r, const gchar* path,
    const gchar* ntext, gpointer udata)
{
  uitree_t      *uitree = udata;
  GtkWidget     *tree;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  GType         coltype;
  int           col;

  if (uitree == NULL) {
    return;
  }

  tree = uitree->uitree.widget;
  col = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (r), "uicolumn"));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  gtk_tree_model_get_iter_from_string (model, &iter, path);
  coltype = gtk_tree_model_get_column_type (model, col);
  if (coltype == G_TYPE_STRING) {
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, ntext, -1);
  }
  if (coltype == G_TYPE_LONG) {
    long val = atol (ntext);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, val, -1);
  }
}
