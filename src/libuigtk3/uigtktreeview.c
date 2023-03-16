/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "callback.h"
#include "mdebug.h"
#include "tagdef.h"
#include "uiclass.h"

#include "ui/uibox.h"
#include "ui/uigeneral.h"
#include "ui/uitreeview.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

static GType * uiAppendType (GType *types, int ncol, int type);
static void uiTreeViewEditedCallback (GtkCellRendererText* r, const gchar* path, const gchar* ntext, gpointer udata);
static void uiTreeViewRowActiveHandler (GtkTreeView* tv, GtkTreePath* path, GtkTreeViewColumn* column, gpointer udata);

typedef struct uitree {
  UIWidget          uitree;
  GtkTreeSelection  *sel;
  callback_t        *editedcb;
  GtkTreeIter       selectiter;
  GtkTreeModel      *model;
  callback_t        *rowactivecb;
  bool              selectset;
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
  uitree->sel = sel;
  uitree->selectset = false;
  uitree->model = NULL;
  uitree->rowactivecb = NULL;
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

void
uiTreeViewSetRowActivatedCallback (uitree_t *uitree, callback_t *cb)
{
  if (uitree == NULL) {
    return;
  }
  uitree->rowactivecb = cb;
  g_signal_connect (uitree->uitree.widget, "row-activated",
      G_CALLBACK (uiTreeViewRowActiveHandler), uitree);
}

UIWidget *
uiTreeViewGetUIWidget (uitree_t *uitree)
{
  if (uitree == NULL) {
    return NULL;
  }

  return &uitree->uitree;
}

void
uiTreeViewAppendColumn (uitree_t *uitree, int coldisp, const char *title, ...)
{
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  va_list           args;
  int               coltype;
  int               col;
  char              *gtkcoltype = "text";
//  bool              first = true;

  if (uitree == NULL) {
    return;
  }
  if (! uiutilsUIWidgetSet (&uitree->uitree)) {
    return;
  }

  column = gtk_tree_view_column_new ();
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);

  va_start (args, title);
  coltype = va_arg (args, int);
  while (coltype != -1) {
    switch (coltype) {
      case TREE_COL_MODE_TEXT: {
        gtkcoltype = "text";
        break;
      }
      case TREE_COL_MODE_EDITABLE: {
        gtkcoltype = "editable";
        break;
      }
      case TREE_COL_MODE_FONT: {
        gtkcoltype = "font";
        break;
      }
      case TREE_COL_MODE_MARKUP: {
        gtkcoltype = "markup";
        break;
      }
      case TREE_COL_MODE_ADJUSTMENT: {
        gtkcoltype = "adjustment";
        break;
      }
      case TREE_COL_MODE_DIGITS: {
        gtkcoltype = "digits";
        break;
      }
      case TREE_COL_MODE_ACTIVE: {
        gtkcoltype = "active";
        break;
      }
      case TREE_COL_MODE_FOREGROUND: {
        gtkcoltype = "foreground";
        break;
      }
      case TREE_COL_MODE_FOREGROUND_SET: {
        gtkcoltype = "foreground-set";
        break;
      }
    }

    col = va_arg (args, int);
//    if (first) {
//      g_object_set_data (G_OBJECT (renderer), "uicolumn", GUINT_TO_POINTER (col));
//      first = false;
//    }
    gtk_tree_view_column_add_attribute (column, renderer, gtkcoltype, col);

    coltype = va_arg (args, int);
  }
  va_end (args);

  gtk_tree_view_column_set_sizing (column, coldisp);
  gtk_tree_view_column_set_title (column, title);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitree->uitree.widget), column);
}

void
uiTreeViewCreateValueStore (uitree_t *uitree, int colmax, ...)
{
  GtkListStore  *store = NULL;
  GType         *types;
  va_list       args;
  int           tval;

  if (uitree == NULL) {
    return;
  }
  if (! uiutilsUIWidgetSet (&uitree->uitree)) {
    return;
  }
  if (colmax <= 0 || colmax > 90) {
    return;
  }

  types = mdmalloc (sizeof (GType) * colmax);

  if (types != NULL) {
    va_start (args, colmax);
    for (int i = 0; i < colmax; ++i) {
      tval = va_arg (args, int);
      if (tval == TREE_TYPE_END) {
        fprintf (stderr, "uiTreeViewCreateValueStore: not enough values\n");
        break;
      }
      types [i] = tval;
    }
    tval = va_arg (args, int);
    if (tval != TREE_TYPE_END) {
      fprintf (stderr, "uiTreeViewCreateValueStore: too many values\n");
    }
    va_end (args);

    store = gtk_list_store_newv (colmax, types);
    gtk_tree_view_set_model (GTK_TREE_VIEW (uitree->uitree.widget),
        GTK_TREE_MODEL (store));
    g_object_unref (store);
    mdfree (types);
    uitree->model = gtk_tree_view_get_model (GTK_TREE_VIEW (uitree->uitree.widget));
  }
}

void
uiTreeViewCreateValueStoreFromList (uitree_t *uitree, int colmax, int *typelist)
{
  GtkListStore  *store = NULL;
  GType         *types;

  if (uitree == NULL) {
    return;
  }
  if (! uiutilsUIWidgetSet (&uitree->uitree)) {
    return;
  }
  if (colmax <= 0 || colmax > 90) {
    return;
  }

  types = mdmalloc (sizeof (GType) * colmax);

  if (types != NULL) {
    for (int i = 0; i < colmax; ++i) {
      if (typelist [i] == TREE_TYPE_IMAGE) {
        types [i] = GDK_TYPE_PIXBUF;
      } else {
        types [i] = typelist [i];
      }
    }

    store = gtk_list_store_newv (colmax, types);
    gtk_tree_view_set_model (GTK_TREE_VIEW (uitree->uitree.widget),
        GTK_TREE_MODEL (store));
    g_object_unref (store);
    mdfree (types);
    uitree->model = gtk_tree_view_get_model (GTK_TREE_VIEW (uitree->uitree.widget));
  }
}

void
uiTreeViewAppendValueStore (uitree_t *uitree)
{
  GtkListStore  *store = NULL;

  if (uitree == NULL) {
    return;
  }
  if (uitree->model == NULL) {
    return;
  }
  if (! uiutilsUIWidgetSet (&uitree->uitree)) {
    return;
  }

  store = GTK_LIST_STORE (uitree->model);
  gtk_list_store_append (store, &uitree->selectiter);
  uitree->selectset = true;
}

void
uiTreeViewInsertValueStore (uitree_t *uitree)
{
  GtkListStore  *store = NULL;
  GtkTreeIter   titer;

  if (uitree == NULL) {
    return;
  }
  if (! uiutilsUIWidgetSet (&uitree->uitree)) {
    return;
  }
  if (uitree->model == NULL) {
    return;
  }
  if (! uitree->selectset) {
    return;
  }

  store = GTK_LIST_STORE (uitree->model);
  memcpy (&titer, &uitree->selectiter, sizeof (GtkTreeIter));
  gtk_list_store_insert_before (store, &uitree->selectiter, &titer);
  uitree->selectset = true;
}

void
uiTreeViewSetValues (uitree_t *uitree, ...)
{
  GtkListStore  *store = NULL;
  va_list       args;

  if (uitree == NULL) {
    return;
  }
  if (uitree->model == NULL) {
    return;
  }
  if (! uiutilsUIWidgetSet (&uitree->uitree)) {
    return;
  }

  store = GTK_LIST_STORE (uitree->model);
  va_start (args, uitree);
  gtk_list_store_set_valist (store, &uitree->selectiter, args);
  va_end (args);
  uitree->selectset = true;
}

int
uiTreeViewGetSelectCount (uitree_t *uitree)
{
  GtkTreeSelection  *sel;
  int               count;

  uitree->selectset = false;
  if (uitree == NULL) {
    return 0;
  }
  if (uitree->model == NULL) {
    return 0;
  }
  if (! uiutilsUIWidgetSet (&uitree->uitree)) {
    return 0;
  }

  sel = uitree->sel;
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count == 0) {
    uitree->selectset = false;
  }
  if (count == 1) {
    /* this only works if the treeview is in single-selection mode */
    gtk_tree_selection_get_selected (sel, &uitree->model, &uitree->selectiter);
    uitree->selectset = true;
  }
  return count;
}

int
uiTreeViewGetSelectionIndex (uitree_t *uitree)
{
  GtkTreePath       *path;
  char              *pathstr;
  int               idx;

  if (uitree == NULL) {
    return -1;
  }
  if (uitree->model == NULL) {
    return -1;
  }
  if (! uiutilsUIWidgetSet (&uitree->uitree)) {
    return -1;
  }
  if (! uitree->selectset) {
    return -1;
  }

  idx = 0;
  path = gtk_tree_model_get_path (uitree->model, &uitree->selectiter);
  mdextalloc (path);
  if (path != NULL) {
    pathstr = gtk_tree_path_to_string (path);
    mdextalloc (pathstr);
    sscanf (pathstr, "%d", &idx);
    mdextfree (path);
    gtk_tree_path_free (path);
    mdfree (pathstr);       // allocated by gtk
  }

  return idx;
}

void
uiTreeViewSelectCurrent (uitree_t *uitree)
{
  GtkTreePath       *path;

  if (uitree == NULL) {
    return;
  }
  if (uitree->model == NULL) {
    return;
  }
  if (! uiutilsUIWidgetSet (&uitree->uitree)) {
    return;
  }

  path = gtk_tree_model_get_path (uitree->model, &uitree->selectiter);
  mdextalloc (path);
  if (path != NULL) {
    gtk_tree_selection_select_path (uitree->sel, path);
    mdextfree (path);
    gtk_tree_path_free (path);
  }
}

bool
uiTreeViewSelectFirst (uitree_t *uitree)
{
  int   valid = false;

  if (uitree == NULL) {
    return valid;
  }
  if (uitree->model == NULL) {
    return valid;
  }

  valid = gtk_tree_model_get_iter_first (uitree->model, &uitree->selectiter);
  return valid;
}

bool
uiTreeViewSelectNext (uitree_t *uitree)
{
  int   valid = false;

  if (uitree == NULL) {
    return valid;
  }
  if (uitree->model == NULL) {
    return valid;
  }

  valid = gtk_tree_model_iter_next (uitree->model, &uitree->selectiter);
  return valid;
}

bool
uiTreeViewSelectPrevious (uitree_t *uitree)
{
  int   valid = false;

  if (uitree == NULL) {
    return valid;
  }
  if (uitree->model == NULL) {
    return valid;
  }

  valid = gtk_tree_model_iter_previous (uitree->model, &uitree->selectiter);
  return valid;
}

int
uiTreeViewSelectDefault (uitree_t *uitree)
{
  int               count = 0;
  GtkTreeIter       iter;
  GtkTreeModel      *model;


  count = uiTreeViewGetSelection (uitree, &model, &iter);
  if (count != 1) {
    uiTreeViewSelectionSet (uitree, 0);
  }

  return count;
}

long
uiTreeViewGetValue (uitree_t *uitree, int col)
{
  glong     idx;

  if (uitree == NULL) {
    return -1;
  }
  if (uitree->model == NULL) {
    return -1;
  }

  gtk_tree_model_get (uitree->model, &uitree->selectiter, col, &idx, -1);
  return idx;
}

const char *
uiTreeViewGetValueStr (uitree_t *uitree, int col)
{
  char    *str;

  if (uitree == NULL) {
    return NULL;
  }
  if (uitree->model == NULL) {
    return NULL;
  }

  gtk_tree_model_get (uitree->model, &uitree->selectiter, col, &str, -1);
  return str;
}

int
uiTreeViewGetSelection (uitree_t *uitree, GtkTreeModel **model, GtkTreeIter *iter)
{
  GtkTreeSelection  *sel;
  int               count;

  if (uitree == NULL) {
    return 0;
  }
  if (! uiutilsUIWidgetSet (&uitree->uitree)) {
    return 0;
  }

  sel = uitree->sel;
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

  sel = uitree->sel;
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
  uiWidgetSetClass (&uitree->uitree, TREEVIEW_DARK_CLASS);
}

void
uiTreeViewDisableSingleClick (uitree_t *uitree)
{
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (uitree->uitree.widget), FALSE);
}

void
uiTreeViewSelectionSetMode (uitree_t *uitree, int mode)
{
  gtk_tree_selection_set_mode (uitree->sel, mode);
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
    gtk_tree_selection_select_path (uitree->sel, path);
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
  if (uitree->sel == NULL) {
    return count;
  }

  count = gtk_tree_selection_count_selected_rows (uitree->sel);
  return count;
}

/* used by configuration */

void
uiTreeViewSetEditedCallback (uitree_t *uitree, callback_t *uicb)
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
  GType     type;

  type = valtype;
  if (valtype == TREE_TYPE_IMAGE) {
    type = GDK_TYPE_PIXBUF;
  }
  if (type == TREE_TYPE_NUM) {
    /* despite being a numeric type, the display needs a string */
    /* so that empty values can be displayed */
    type = TREE_TYPE_STRING;
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

static void
uiTreeViewRowActiveHandler (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata)
{
  uitree_t    *uitree = udata;
  int         count;

  if (uitree == NULL) {
    return;
  }

  count = uiTreeViewGetSelectCount (uitree);
  if (count == 1) {
    gtk_tree_selection_get_selected (uitree->sel, &uitree->model, &uitree->selectiter);
  }
  if (uitree->rowactivecb != NULL) {
    callbackHandler (uitree->rowactivecb);
  }
}


