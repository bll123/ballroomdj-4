/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "callback.h"
#include "mdebug.h"
#include "tagdef.h"
#include "uiclass.h"
#include "uiwcont.h"

#include "ui-gtk3.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uitreeview.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

enum {
  SELECT_PROCESS_NONE,
  SELECT_PROCESS_CLEAR_ALL,
  SELECT_PROCESS_GET_SELECT_ITER,
};

enum {
  TV_CB_SCROLL_EVENT,
  TV_CB_SIZE_CHG,
  TV_CB_SELECT_CHG,
  TV_CB_ROW_ACTIVE,
  TV_CB_FOREACH,
  TV_CB_EDITED,
  TV_CB_BUTTON_3,
  TV_CB_MAX,
};


typedef struct uitree {
  GtkTreeSelection  *sel;
  GtkTreeIter       selectiter;
  GtkTreeIter       valueiter;
  GtkTreeIter       selectforeachiter;
  GtkTreeModel      *model;
  GtkTreeViewColumn *activeColumn;
  GtkEventController *scrollController;
  callback_t        *callbacks [TV_CB_MAX];
  int               savedrow;
  int               selectprocessmode;
  int               selmode;
  int               minwidth;           // prep for append column
  int               ellipsizeColumn;    // prep for append column
  int               colorcol;             // prep for append column
  int               colorsetcol;          // prep for append column
  int               radiorow;
  int               activecol;
  int               lastTreeSize;
  int               valueRowCount;
  bool              selectset : 1;
  bool              savedselectset : 1;
  bool              valueiterset : 1;
} uitree_t;

static void uiTreeViewEditedHandler (GtkCellRendererText* r, const gchar* path, const gchar* ntext, gpointer udata);
static void uiTreeViewCheckboxHandler (GtkCellRendererToggle *renderer, gchar *spath, gpointer udata);
static void uiTreeViewRadioHandler (GtkCellRendererToggle *renderer, gchar *spath, gpointer udata);
static void uiTreeViewRowActiveHandler (GtkTreeView* tv, GtkTreePath* path, GtkTreeViewColumn* column, gpointer udata);
static gboolean uiTreeViewClickHandler (GtkWidget *tv, GdkEventButton *event, gpointer udata);
static gboolean uiTreeViewForeachHandler (GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer udata);
static void uiTreeViewSelectForeachHandler (GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer udata);
static GType uiTreeViewConvertTreeType (int type);
static void uiTreeViewSelectChangedHandler (GtkTreeSelection *sel, gpointer udata);
static void uiTreeViewSizeChangeHandler (GtkWidget* w, GtkAllocation* allocation, gpointer udata);
static gboolean uiTreeViewScrollEventHandler (GtkWidget* tv, GdkEventScroll *event, gpointer udata);

uiwcont_t *
uiCreateTreeView (void)
{
  uiwcont_t         *uiwidget;
  uitree_t          *uitree;
  GtkWidget         *tree;
  GtkTreeSelection  *sel;

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_TREE;
  uiwidget->wtype = WCONT_T_TREE;

  tree = gtk_tree_view_new ();
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (tree), FALSE);
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (tree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);

  gtk_widget_set_halign (tree, GTK_ALIGN_START);
  gtk_widget_set_hexpand (tree, FALSE);
  gtk_widget_set_vexpand (tree, FALSE);

  uitree = mdmalloc (sizeof (uitree_t));
  uitree->sel = sel;
  uitree->selmode = SELECT_SINGLE;
  uitree->selectset = false;
  uitree->savedrow = TREE_NO_ROW;
  uitree->savedselectset = false;
  uitree->valueiterset = false;
  uitree->model = NULL;
  uitree->activeColumn = NULL;
  uitree->scrollController = NULL;
  for (int i = 0; i < TV_CB_MAX; ++i) {
    uitree->callbacks [i] = NULL;
  }
  uitree->minwidth = TREE_NO_MIN_WIDTH;
  uitree->ellipsizeColumn = TREE_NO_COLUMN;
  uitree->colorcol = TREE_NO_COLUMN;
  uitree->colorsetcol = TREE_NO_COLUMN;
  uitree->radiorow = TREE_NO_ROW;
  uitree->activecol = TREE_NO_COLUMN;
  uitree->selectprocessmode = SELECT_PROCESS_NONE;
  uitree->lastTreeSize = -1;
  uitree->valueRowCount = 0;

  uiwidget->uidata.widget = tree;
  uiwidget->uidata.packwidget = tree;
  uiwidget->uiint.uitree = uitree;

  uiWidgetSetAllMargins (uiwidget, 2);

  return uiwidget;
}

void
uiTreeViewFree (uiwcont_t *uiwidget)
{
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-free")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;
  mdfree (uitree);
}

void
uiTreeViewEnableHeaders (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-enable-header")) {
    return;
  }

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uiwidget->uidata.widget), TRUE);
}

void
uiTreeViewDisableHeaders (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-disable-headers")) {
    return;
  }

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (uiwidget->uidata.widget), FALSE);
}

void
uiTreeViewDarkBackground (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-dark-bg")) {
    return;
  }

  uiWidgetSetClass (uiwidget, TREEVIEW_DARK_CLASS);
}

void
uiTreeViewDisableSingleClick (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-disable-single-click")) {
    return;
  }

  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (uiwidget->uidata.widget), FALSE);
}

void
uiTreeViewSelectSetMode (uiwcont_t *uiwidget, int mode)
{
  int       gtkmode = GTK_SELECTION_SINGLE;
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-set-mode")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (mode == SELECT_SINGLE) {
    gtkmode = GTK_SELECTION_SINGLE;
  }
  if (mode == SELECT_MULTIPLE) {
    gtkmode = GTK_SELECTION_MULTIPLE;
  }

  gtk_tree_selection_set_mode (uitree->sel, gtkmode);
  uitree->selmode = mode;
}

void
uiTreeViewSetSelectChangedCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-chg-cb")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->callbacks [TV_CB_SELECT_CHG] = cb;
  g_signal_connect (uitree->sel, "changed",
      G_CALLBACK (uiTreeViewSelectChangedHandler), uiwidget);
}

void
uiTreeViewSetSizeChangeCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-set-sz-chg-cb")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->callbacks [TV_CB_SIZE_CHG] = cb;
  g_signal_connect (uiwidget->uidata.widget, "size-allocate",
      G_CALLBACK (uiTreeViewSizeChangeHandler), uiwidget);
}

void
uiTreeViewSetScrollEventCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-set-scroll-event-cb")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->callbacks [TV_CB_SCROLL_EVENT] = cb;
  g_signal_connect (uiwidget->uidata.widget, "scroll-event",
      G_CALLBACK (uiTreeViewScrollEventHandler), uiwidget);
}

void
uiTreeViewSetRowActivatedCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-set-row-activated-cb")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->callbacks [TV_CB_ROW_ACTIVE] = cb;
  g_signal_connect (uiwidget->uidata.widget, "row-activated",
      G_CALLBACK (uiTreeViewRowActiveHandler), uiwidget);
  g_signal_connect (uiwidget->uidata.widget, "button-press-event",
      G_CALLBACK (uiTreeViewClickHandler), uiwidget);
}

void
uiTreeViewSetEditedCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-set-edited-cb")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->callbacks [TV_CB_EDITED] = cb;
}

void
uiTreeViewSetButton3Callback (uiwcont_t *uiwidget, callback_t *cb)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-set-edited-cb")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->callbacks [TV_CB_BUTTON_3] = cb;
}

void
uiTreeViewRadioSetRow (uiwcont_t *uiwidget, int row)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-radio-set-row")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->radiorow = row;
}


void
uiTreeViewPreColumnSetMinWidth (uiwcont_t *uiwidget, int minwidth)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-set-min")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->minwidth = minwidth;
}

void
uiTreeViewPreColumnSetEllipsizeColumn (uiwcont_t *uiwidget, int ellipsizeColumn)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-set-ellipsize")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->ellipsizeColumn = ellipsizeColumn;
}

void
uiTreeViewPreColumnSetColorColumn (uiwcont_t *uiwidget, int colorcol, int colorsetcol)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-set-color")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->colorcol = colorcol;
  uitree->colorsetcol = colorsetcol;
}

void
uiTreeViewAppendColumn (uiwcont_t *uiwidget, int activecol, int widgettype,
    int alignment, int coldisp, const char *title, ...)
{
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  va_list           args;
  int               coltype;
  int               col;
  const char        *gtkcoltype = "text";
  int               gtkcoldisp;
  uitree_t          *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-append-col")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  column = gtk_tree_view_column_new ();
  switch (widgettype) {
    case TREE_WIDGET_TEXT: {
      renderer = gtk_cell_renderer_text_new ();
      break;
    }
    case TREE_WIDGET_SPINBOX: {
      renderer = gtk_cell_renderer_spin_new ();
      alignment = TREE_ALIGN_RIGHT;
      break;
    }
    case TREE_WIDGET_IMAGE: {
      renderer = gtk_cell_renderer_pixbuf_new ();
      alignment = TREE_ALIGN_CENTER;
      break;
    }
    case TREE_WIDGET_CHECKBOX: {
      renderer = gtk_cell_renderer_toggle_new ();
      alignment = TREE_ALIGN_CENTER;
      break;
    }
    case TREE_WIDGET_RADIO: {
      renderer = gtk_cell_renderer_toggle_new ();
      gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
      alignment = TREE_ALIGN_CENTER;
      break;
    }
    default: {
      fprintf (stderr, "ERR: unhandled tree widget: %d\n", widgettype);
      break;
    }
  }

  if (alignment == TREE_ALIGN_RIGHT) {
    gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  }
  if (alignment == TREE_ALIGN_CENTER) {
    gtk_cell_renderer_set_alignment (renderer, 0.5, 0.5);
  }

  if (uitree->minwidth != TREE_NO_MIN_WIDTH) {
    gtk_tree_view_column_set_min_width (column, uitree->minwidth);
    /* only good for one column */
    uitree->minwidth = TREE_NO_MIN_WIDTH;
  }

  gtk_tree_view_column_pack_start (column, renderer, TRUE);

  va_start (args, title);
  coltype = va_arg (args, int);
  while (coltype != TREE_COL_TYPE_END) {
    col = va_arg (args, int);

    switch (coltype) {
      case TREE_COL_TYPE_TEXT: {
        /* the stupid "edited" signal has no way to retrieve the column number */
        /* so set some data in the renderer to save the column number */
        g_object_set_data (G_OBJECT (renderer), "uicolumn", GUINT_TO_POINTER (col));
        gtkcoltype = "text";
        break;
      }
      case TREE_COL_TYPE_EDITABLE: {
        g_signal_connect (renderer, "edited",
            G_CALLBACK (uiTreeViewEditedHandler), uiwidget);
        gtkcoltype = "editable";
        break;
      }
      case TREE_COL_TYPE_FONT: {
        gtkcoltype = "font";
        break;
      }
      case TREE_COL_TYPE_MARKUP: {
        gtkcoltype = "markup";
        break;
      }
      case TREE_COL_TYPE_ADJUSTMENT: {
        gtkcoltype = "adjustment";
        break;
      }
      case TREE_COL_TYPE_DIGITS: {
        gtkcoltype = "digits";
        break;
      }
      case TREE_COL_TYPE_ACTIVE: {
        /* the stupid "toggled" signal has no way to retrieve the column number */
        /* so set some data in the renderer to save the column number */
        g_object_set_data (G_OBJECT (renderer), "uicolumn", GUINT_TO_POINTER (col));
        if (widgettype == TREE_WIDGET_CHECKBOX) {
          g_signal_connect (G_OBJECT(renderer), "toggled",
              G_CALLBACK (uiTreeViewCheckboxHandler), uiwidget);
        }
        if (widgettype == TREE_WIDGET_RADIO) {
          g_signal_connect (G_OBJECT(renderer), "toggled",
              G_CALLBACK (uiTreeViewRadioHandler), uiwidget);
        }
        gtkcoltype = "active";
        break;
      }
      case TREE_COL_TYPE_FOREGROUND: {
        gtkcoltype = "foreground";
        break;
      }
      case TREE_COL_TYPE_FOREGROUND_SET: {
        gtkcoltype = "foreground-set";
        break;
      }
      case TREE_COL_TYPE_IMAGE: {
        gtkcoltype = "pixbuf";
        break;
      }
      default: {
        fprintf (stderr, "ERR: unhandled column mode: %d\n", coltype);
        break;
      }
    }

    gtk_tree_view_column_add_attribute (column, renderer, gtkcoltype, col);

    coltype = va_arg (args, int);
  }
  va_end (args);

  if (uitree->ellipsizeColumn != TREE_NO_COLUMN) {
    gtk_tree_view_column_add_attribute (column, renderer, "ellipsize", uitree->ellipsizeColumn);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_expand (column, TRUE);
    uitree->ellipsizeColumn = TREE_NO_COLUMN;
  }

  if (uitree->colorcol != TREE_NO_COLUMN && uitree->colorsetcol != TREE_NO_COLUMN) {
    gtk_tree_view_column_add_attribute (column, renderer, "foreground", uitree->colorcol);
    gtk_tree_view_column_add_attribute (column, renderer, "foreground-set", uitree->colorsetcol);
    uitree->colorcol = TREE_NO_COLUMN;
    uitree->colorsetcol = TREE_NO_COLUMN;
  }

  gtkcoldisp = GTK_TREE_VIEW_COLUMN_AUTOSIZE;
  switch (coldisp) {
    case TREE_COL_DISP_NORM: {
      gtkcoldisp = GTK_TREE_VIEW_COLUMN_AUTOSIZE;
      break;
    }
    case TREE_COL_DISP_GROW: {
      gtkcoldisp = GTK_TREE_VIEW_COLUMN_GROW_ONLY;
      break;
    }
  }
  gtk_tree_view_column_set_sizing (column, gtkcoldisp);
  if (title != NULL) {
    gtk_tree_view_column_set_title (column, title);
  }
  gtk_tree_view_append_column (GTK_TREE_VIEW (uiwidget->uidata.widget), column);

  if (activecol != TREE_NO_COLUMN) {
    uitree->activeColumn = column;
    uitree->activecol = activecol;
  }
}

void
uiTreeViewColumnSetVisible (uiwcont_t *uiwidget, int col, int flag)
{
  GtkTreeViewColumn   *column = NULL;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-set-visible")) {
    return;
  }

  column = gtk_tree_view_get_column (GTK_TREE_VIEW (uiwidget->uidata.widget), col);
  if (column != NULL) {
    int     val = TRUE;

    if (flag == TREE_COLUMN_HIDDEN) {
      val = FALSE;
    }
    if (flag == TREE_COLUMN_SHOWN) {
      val = TRUE;
    }
    gtk_tree_view_column_set_visible (column, val);
  }
}

void
uiTreeViewCreateValueStore (uiwcont_t *uiwidget, int colmax, ...)
{
  GtkListStore  *store = NULL;
  GType         *types;
  va_list       args;
  int           tval;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-create-store")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (colmax <= 0 || colmax > 90) {
    return;
  }

  types = mdmalloc (sizeof (GType) * colmax);

  if (types != NULL) {
    va_start (args, colmax);
    for (int i = 0; i < colmax; ++i) {
      tval = va_arg (args, int);
      if (tval == TREE_TYPE_END) {
        fprintf (stderr, "ERR: uiTreeViewCreateValueStore: not enough values\n");
        break;
      }
      types [i] = uiTreeViewConvertTreeType (tval);
    }
    tval = va_arg (args, int);
    if (tval != TREE_TYPE_END) {
      fprintf (stderr, "ERR: uiTreeViewCreateValueStore: too many values\n");
    }
    va_end (args);

    store = gtk_list_store_newv (colmax, types);
    gtk_tree_view_set_model (GTK_TREE_VIEW (uiwidget->uidata.widget),
        GTK_TREE_MODEL (store));
    g_object_unref (store);
    mdfree (types);
    uitree->model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidget->uidata.widget));
  }

  uitree->valueRowCount = 0;
}

void
uiTreeViewCreateValueStoreFromList (uiwcont_t *uiwidget, int colmax, int *typelist)
{
  GtkListStore  *store = NULL;
  GType         *types;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-create-store-list")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (colmax <= 0 || colmax > 90) {
    return;
  }

  types = mdmalloc (sizeof (GType) * colmax);

  if (types != NULL) {
    for (int i = 0; i < colmax; ++i) {
      types [i] = uiTreeViewConvertTreeType (typelist [i]);
    }

    store = gtk_list_store_newv (colmax, types);
    gtk_tree_view_set_model (GTK_TREE_VIEW (uiwidget->uidata.widget),
        GTK_TREE_MODEL (store));
    g_object_unref (store);
    mdfree (types);
    uitree->model = gtk_tree_view_get_model (GTK_TREE_VIEW (uiwidget->uidata.widget));
  }

  uitree->valueRowCount = 0;
}

void
uiTreeViewValueAppend (uiwcont_t *uiwidget)
{
  GtkListStore  *store = NULL;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-value-append")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  store = GTK_LIST_STORE (uitree->model);
  gtk_list_store_append (store, &uitree->selectiter);
  uitree->selectset = true;
  ++uitree->valueRowCount;
}

void
uiTreeViewValueInsertBefore (uiwcont_t *uiwidget)
{
  GtkListStore  *store = NULL;
  GtkTreeIter   titer;
  GtkTreeIter   *titerp;
  int           count;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-ins-before")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  store = GTK_LIST_STORE (uitree->model);
  count = gtk_tree_selection_count_selected_rows (uitree->sel);
  if (count == 0 || ! uitree->selectset) {
    titerp = NULL;
  } else {
    memcpy (&titer, &uitree->selectiter, sizeof (GtkTreeIter));
    titerp = &titer;
  }
  gtk_list_store_insert_before (store, &uitree->selectiter, titerp);
  gtk_tree_selection_select_iter (uitree->sel, &uitree->selectiter);
  uitree->selectset = true;
  ++uitree->valueRowCount;
}

void
uiTreeViewValueInsertAfter (uiwcont_t *uiwidget)
{
  GtkListStore  *store = NULL;
  GtkTreeIter   titer;
  GtkTreeIter   *titerp;
  int           count;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-ins-after")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  store = GTK_LIST_STORE (uitree->model);
  count = gtk_tree_selection_count_selected_rows (uitree->sel);
  if (count == 0 || ! uitree->selectset) {
    titerp = NULL;
  } else {
    memcpy (&titer, &uitree->selectiter, sizeof (GtkTreeIter));
    titerp = &titer;
  }
  gtk_list_store_insert_after (store, &uitree->selectiter, titerp);
  gtk_tree_selection_select_iter (uitree->sel, &uitree->selectiter);
  uitree->selectset = true;
  ++uitree->valueRowCount;
}

void
uiTreeViewValueRemove (uiwcont_t *uiwidget)
{
  int     count = 0;
  int     idx;
  bool    valid = false;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-value-remove")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }
  if (! uitree->selectset) {
    return;
  }

  idx = uiTreeViewSelectGetIndex (uiwidget);
  valid = gtk_list_store_remove (GTK_LIST_STORE (uitree->model), &uitree->selectiter);
  --uitree->valueRowCount;
  uitree->selectset = false;
  count = gtk_tree_selection_count_selected_rows (uitree->sel);
  if (count == 1 && uitree->selmode == SELECT_SINGLE) {
    valid = gtk_tree_selection_get_selected (uitree->sel, &uitree->model, &uitree->selectiter);
    if (valid) {
      uitree->selectset = true;
    }
  }

  /* count == 0 will default to ! valid */
  if (! valid && idx > 0) {
    --idx;
    if (idx <= 0) { idx = 0; }
    uiTreeViewSelectSet (uiwidget, idx);
  }
}

void
uiTreeViewValueClear (uiwcont_t *uiwidget, int startrow)
{
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-value-clear")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  if (startrow == 0) {
    gtk_list_store_clear (GTK_LIST_STORE (uitree->model));
    uitree->valueRowCount = 0;
  } else if (startrow > 0) {
    char        tbuff [40];
    GtkTreeIter iter;
    bool        valid = false;

    snprintf (tbuff, sizeof (tbuff), "%d", startrow);
    valid = gtk_tree_model_get_iter_from_string (uitree->model, &iter, tbuff);
    while (valid) {
      valid = gtk_list_store_remove (GTK_LIST_STORE (uitree->model), &iter);
      --uitree->valueRowCount;
    }
  }
}

/* must be called before set-values if the value iter is being used */
void
uiTreeViewSetValueEllipsize (uiwcont_t *uiwidget, int col)
{
  GtkListStore  *store = NULL;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-value-ellipsize")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  store = GTK_LIST_STORE (uitree->model);

  if (uitree->valueiterset) {
    gtk_list_store_set (store, &uitree->valueiter, col,
        (int) PANGO_ELLIPSIZE_END, TREE_VALUE_END);
    /* do not reset the valueiterset flag */
  } else if (uitree->selectset) {
    gtk_list_store_set (store, &uitree->selectiter, col,
        (int) PANGO_ELLIPSIZE_END, TREE_VALUE_END);
  }
}

void
uiTreeViewSetValues (uiwcont_t *uiwidget, ...)
{
  GtkListStore  *store = NULL;
  va_list       args;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-set-values")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  store = GTK_LIST_STORE (uitree->model);
  va_start (args, uiwidget);
  if (uitree->valueiterset) {
    gtk_list_store_set_valist (store, &uitree->valueiter, args);
  } else if (uitree->selectset) {
    gtk_list_store_set_valist (store, &uitree->selectiter, args);
  }
  va_end (args);
}

int
uiTreeViewSelectGetCount (uiwcont_t *uiwidget)
{
  int       count;
  bool      valid;
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-sel-get-count")) {
    return 0;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return 0;
  }

  count = gtk_tree_selection_count_selected_rows (uitree->sel);
  /* don't muck up uitree->selectset if in multi-selection mode */
  if (count == 0 && uitree->selmode == SELECT_SINGLE) {
    uitree->selectset = false;
  }
  if (count == 1 && uitree->selmode == SELECT_SINGLE) {
    uitree->selectset = false;
    /* this only works if the treeview is in single-selection mode */
    valid = gtk_tree_selection_get_selected (uitree->sel, &uitree->model, &uitree->selectiter);
    if (valid) {
      uitree->selectset = true;
    }
  }
  return count;
}

int
uiTreeViewSelectGetIndex (uiwcont_t *uiwidget)
{
  GtkTreePath   *path;
  char          *pathstr;
  int           idx;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-sel-get-index")) {
    return TREE_NO_ROW;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return TREE_NO_ROW;
  }
  if (! uitree->selectset) {
    return TREE_NO_ROW;
  }

  idx = TREE_NO_ROW;
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

/* makes sure that the stored selectiter is pointing to the current selection */
/* coded for both select-mode single and multiple */
/* makes sure the iterator is actually selected and selectiter is set */
void
uiTreeViewSelectCurrent (uiwcont_t *uiwidget)
{
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-current")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  /* select-get-count will handle the select-single case */
  if (uitree->selmode == SELECT_SINGLE) {
    uiTreeViewSelectGetCount (uiwidget);
  }
  if (uitree->selmode == SELECT_MULTIPLE) {
    uitree->selectset = false;
    uitree->selectprocessmode = SELECT_PROCESS_GET_SELECT_ITER;
    uiTreeViewSelectForeach (uiwidget, NULL);
    uitree->selectprocessmode = SELECT_PROCESS_NONE;
    if (uitree->selectset) {
      gtk_tree_selection_select_iter (uitree->sel, &uitree->selectiter);
    }
  }
}

bool
uiTreeViewSelectFirst (uiwcont_t *uiwidget)
{
  int         valid = false;
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-first")) {
    return valid;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return valid;
  }

  if (uitree->selmode == SELECT_MULTIPLE && uitree->selectset) {
    gtk_tree_selection_unselect_iter (uitree->sel, &uitree->selectiter);
  }
  uitree->selectset = false;
  valid = gtk_tree_model_get_iter_first (uitree->model, &uitree->selectiter);
  if (valid) {
    uitree->selectset = true;
    gtk_tree_selection_select_iter (uitree->sel, &uitree->selectiter);
  }
  return valid;
}

bool
uiTreeViewSelectNext (uiwcont_t *uiwidget)
{
  int         valid = false;
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-next")) {
    return valid;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return valid;
  }

  if (uitree->selmode == SELECT_MULTIPLE && uitree->selectset) {
    gtk_tree_selection_unselect_iter (uitree->sel, &uitree->selectiter);
  }
  uitree->selectset = false;
  valid = gtk_tree_model_iter_next (uitree->model, &uitree->selectiter);
  if (valid) {
    uitree->selectset = true;
    gtk_tree_selection_select_iter (uitree->sel, &uitree->selectiter);
  }
  return valid;
}

bool
uiTreeViewSelectPrevious (uiwcont_t *uiwidget)
{
  int         valid = false;
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-previous")) {
    return valid;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return valid;
  }

  if (uitree->selmode == SELECT_MULTIPLE && uitree->selectset) {
    gtk_tree_selection_unselect_iter (uitree->sel, &uitree->selectiter);
  }
  uitree->selectset = false;
  valid = gtk_tree_model_iter_previous (uitree->model, &uitree->selectiter);
  if (valid) {
    uitree->selectset = true;
    gtk_tree_selection_select_iter (uitree->sel, &uitree->selectiter);
  }
  return valid;
}

void
uiTreeViewSelectDefault (uiwcont_t *uiwidget)
{
  int       count = 0;
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-default")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  count = uiTreeViewSelectGetCount (uiwidget);
  if (count != 1) {
    uiTreeViewSelectSet (uiwidget, 0);
  }
}

void
uiTreeViewSelectSave (uiwcont_t *uiwidget)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-save")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }
  if (! uitree->selectset) {
    return;
  }

  if (uitree->selmode == SELECT_SINGLE) {
    uiTreeViewSelectCurrent (uiwidget);
    uitree->savedrow = uiTreeViewSelectGetIndex (uiwidget);
    uitree->savedselectset = uitree->selectset;
  }
}

void
uiTreeViewSelectRestore (uiwcont_t *uiwidget)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-restore")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->selmode == SELECT_SINGLE) {
    uitree->selectset = uitree->savedselectset;
    if (uitree->selectset && uitree->savedrow != TREE_NO_ROW) {
      uiTreeViewSelectSet (uiwidget, uitree->savedrow);
    }
    uitree->savedrow = TREE_NO_ROW;
  }
}

void
uiTreeViewSelectClear (uiwcont_t *uiwidget)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-clear")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }
  if (! uitree->selectset) {
    return;
  }

  gtk_tree_selection_unselect_iter (uitree->sel, &uitree->selectiter);
  uitree->selectset = false;
}

/* use when the select mode is select-multiple */
void
uiTreeViewSelectClearAll (uiwcont_t *uiwidget)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-clear-all")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->selectprocessmode = SELECT_PROCESS_CLEAR_ALL;
  uiTreeViewSelectForeach (uiwidget, NULL);
  uitree->selectprocessmode = SELECT_PROCESS_NONE;
}

void
uiTreeViewSelectForeach (uiwcont_t *uiwidget, callback_t *cb)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-foreach")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  uitree->callbacks [TV_CB_FOREACH] = cb;
  gtk_tree_selection_selected_foreach (uitree->sel,
      uiTreeViewSelectForeachHandler, uiwidget);
}

void
uiTreeViewMoveBefore (uiwcont_t *uiwidget)
{
  GtkTreeIter   citer;
  bool          valid = false;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-move-before")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  memcpy (&citer, &uitree->selectiter, sizeof (citer));
  valid = gtk_tree_model_iter_previous (uitree->model, &uitree->selectiter);
  if (valid) {
    gtk_list_store_move_before (GTK_LIST_STORE (uitree->model),
        &citer, &uitree->selectiter);
    uitree->selectset = true;
  }
}

void
uiTreeViewMoveAfter (uiwcont_t *uiwidget)
{
  GtkTreeIter   citer;
  bool          valid = false;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-move-after")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  memcpy (&citer, &uitree->selectiter, sizeof (citer));
  valid = gtk_tree_model_iter_next (uitree->model, &uitree->selectiter);
  if (valid) {
    gtk_list_store_move_after (GTK_LIST_STORE (uitree->model),
        &citer, &uitree->selectiter);
    uitree->selectset = true;
  }
}

/* gets the value for the selected row */
long
uiTreeViewGetValue (uiwcont_t *uiwidget, int col)
{
  glong       val;
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-get-value")) {
    return -1;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return -1;
  }

  if (uitree->selmode == SELECT_MULTIPLE) {
    uitree->selectset = false;
    uitree->selectprocessmode = SELECT_PROCESS_GET_SELECT_ITER;
    uiTreeViewSelectForeach (uiwidget, NULL);
    uitree->selectprocessmode = SELECT_PROCESS_NONE;
  }

  if (! uitree->selectset) {
    return -1;
  }

  gtk_tree_model_get (uitree->model, &uitree->selectiter, col, &val, -1);
  return val;
}

/* gets the string value for the selected row */
char *
uiTreeViewGetValueStr (uiwcont_t *uiwidget, int col)
{
  char      *str;
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-get-value-str")) {
    return NULL;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return NULL;
  }
  if (! uitree->selectset) {
    return NULL;
  }

  gtk_tree_model_get (uitree->model, &uitree->selectiter, col, &str, -1);
  mdextalloc (str);
  return str;
}

/* gets the value for the row just processed by select-foreach */
long
uiTreeViewSelectForeachGetValue (uiwcont_t *uiwidget, int col)
{
  glong     val;
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-foreach-get-value")) {
    return -1;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return -1;
  }

  gtk_tree_model_get (uitree->model, &uitree->selectforeachiter, col, &val, -1);
  return val;
}

void
uiTreeViewForeach (uiwcont_t *uiwidget, callback_t *cb)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-foreach")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  /* save the selection iter */
  /* the foreach handler sets it so that data can be retrieved */
  uiTreeViewSelectSave (uiwidget);
  uitree->callbacks [TV_CB_FOREACH] = cb;
  gtk_tree_model_foreach (uitree->model, uiTreeViewForeachHandler, uiwidget);
  uiTreeViewSelectRestore (uiwidget);
}

void
uiTreeViewSelectSet (uiwcont_t *uiwidget, int row)
{
  char        tbuff [40];
  GtkTreePath *path = NULL;
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-set")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->selectset = false;

  if (uitree->model == NULL) {
    return;
  }
  if (row < 0) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "%d", row);
  path = gtk_tree_path_new_from_string (tbuff);
  mdextalloc (path);
  if (path != NULL) {
    bool    valid = false;

    gtk_tree_selection_select_path (uitree->sel, path);
    if (uitree->selmode == SELECT_SINGLE) {
      valid = gtk_tree_selection_get_selected (uitree->sel, &uitree->model, &uitree->selectiter);
    }
    if (uitree->selmode == SELECT_MULTIPLE) {
      valid = gtk_tree_model_get_iter (uitree->model, &uitree->selectiter, path);
    }
    if (valid) {
      uitree->selectset = true;
    }
    mdextfree (path);
    gtk_tree_path_free (path);
  }
}

void
uiTreeViewValueIteratorSet (uiwcont_t *uiwidget, int row)
{
  char      tbuff [40];
  bool      valid = false;
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-value-iter-set")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->valueiterset = false;

  if (uitree->model == NULL) {
    return;
  }
  if (row < 0) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "%d", row);
  valid = gtk_tree_model_get_iter_from_string (uitree->model, &uitree->valueiter, tbuff);
  if (valid) {
    uitree->valueiterset = true;
  }
}

void
uiTreeViewValueIteratorClear (uiwcont_t *uiwidget)
{
  uitree_t  *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-value-iter-clear")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  uitree->valueiterset = false;
}

void
uiTreeViewScrollToCell (uiwcont_t *uiwidget)
{
  GtkTreePath *path = NULL;
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-scroll-to-cell")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  if (! GTK_IS_TREE_VIEW (uiwidget->uidata.widget)) {
    return;
  }

  path = gtk_tree_model_get_path (uitree->model, &uitree->selectiter);
  mdextalloc (path);
  if (path != NULL) {
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (uiwidget->uidata.widget),
        path, NULL, FALSE, 0.0, 0.0);
    mdextfree (path);
    gtk_tree_path_free (path);
  }
}

void
uiTreeViewAttachScrollController (uiwcont_t *uiwidget, double upper)
{
  GtkAdjustment   *adjustment;
  uitree_t        *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-attach-scroll")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  adjustment = gtk_scrollable_get_vadjustment (
      GTK_SCROLLABLE (uiwidget->uidata.widget));
  gtk_adjustment_set_upper (adjustment, upper);
  uitree->scrollController =
      gtk_event_controller_scroll_new (uiwidget->uidata.widget,
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL |
      GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
  gtk_widget_add_events (uiwidget->uidata.widget, GDK_SCROLL_MASK);
}

int
uiTreeViewGetDragDropRow (uiwcont_t *uiwidget, int x, int y)
{
  int                     row;
  GtkTreePath             *path;
  GtkTreeViewDropPosition pos;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-get-drag-drop-row")) {
    return -1;
  }

  row = -1;
  if (gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (uiwidget->uidata.widget),
      x, y, &path, &pos)) {
    char      *pathstr;

    mdextalloc (path);
    pathstr = gtk_tree_path_to_string (path);
    mdextalloc (pathstr);
    row = atoi (pathstr);
    if (pos == GTK_TREE_VIEW_DROP_BEFORE && row > 0) {
      --row;
    }
    mdfree (pathstr);
    mdextfree (path);
    gtk_tree_path_free (path);
  }

  return row;
}

/* internal routines */

/* used by the editable column routines */
static void
uiTreeViewEditedHandler (GtkCellRendererText* r, const gchar* pathstr,
    const gchar* ntext, gpointer udata)
{
  uiwcont_t     *uiwidget = udata;
  GtkTreeIter   iter;
  GType         coltype;
  int           col;
  char          *oldstr = NULL;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-edited-handler")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  /* retrieve the column number from the 'uicolumn' value set when */
  /* the column was created */
  col = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (r), "uicolumn"));

  gtk_tree_model_get_iter_from_string (uitree->model, &iter, pathstr);
  coltype = gtk_tree_model_get_column_type (uitree->model, col);

  if (coltype == G_TYPE_STRING) {
    gtk_tree_model_get (uitree->model, &iter, col, &oldstr, -1);
    mdextalloc (oldstr);
    gtk_list_store_set (GTK_LIST_STORE (uitree->model), &iter, col, ntext, -1);
  }
  if (coltype == G_TYPE_LONG) {
    long val = atol (ntext);
    gtk_list_store_set (GTK_LIST_STORE (uitree->model), &iter, col, val, -1);
  }

  if (uitree->callbacks [TV_CB_EDITED] != NULL) {
    int   rc;

    rc = callbackHandlerLong (uitree->callbacks [TV_CB_EDITED], col);
    if (oldstr != NULL && rc == UICB_STOP) {
      gtk_list_store_set (GTK_LIST_STORE (uitree->model), &iter, col, oldstr, -1);
    }
  }
  dataFree (oldstr);
}

static void
uiTreeViewCheckboxHandler (GtkCellRendererToggle *r,
    gchar *pathstr, gpointer udata)
{
  uiwcont_t     *uiwidget = udata;
  GtkWidget     *tree;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  int           col;
  gint          val;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-chkbox-handler")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  tree = uiwidget->uidata.widget;

  /* retrieve the column number from the 'uicolumn' value set when */
  /* the column was created */
  col = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (r), "uicolumn"));

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  gtk_tree_model_get_iter_from_string (model, &iter, pathstr);
  gtk_tree_model_get (model, &iter, col, &val, -1);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, !val, -1);

  if (uitree->callbacks [TV_CB_EDITED] != NULL) {
    callbackHandlerLong (uitree->callbacks [TV_CB_EDITED], col);
  }
}

static void
uiTreeViewRadioHandler (GtkCellRendererToggle *r,
    gchar *pathstr, gpointer udata)
{
  uiwcont_t     *uiwidget = udata;
  GtkWidget     *tree;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  int           col;
  uitree_t      *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-radio-handler")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  tree = uiwidget->uidata.widget;

  /* retrieve the column number from the 'uicolumn' value set when */
  /* the column was created */
  col = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (r), "uicolumn"));

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  gtk_tree_model_get_iter_from_string (model, &iter, pathstr);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, 1, -1);

  if (uitree->radiorow != TREE_NO_ROW) {
    char  tmp [40];

    snprintf (tmp, sizeof (tmp), "%d", uitree->radiorow);
    gtk_tree_model_get_iter_from_string (model, &iter, tmp);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, 0, -1);
  }

  uitree->radiorow = atoi (pathstr);

  if (uitree->callbacks [TV_CB_EDITED] != NULL) {
    callbackHandlerLong (uitree->callbacks [TV_CB_EDITED], col);
  }
}

static void
uiTreeViewRowActiveHandler (GtkTreeView* tv, GtkTreePath* path,
    GtkTreeViewColumn* column, gpointer udata)
{
  uiwcont_t   *uiwidget = udata;
  int         count;
  int         col = TREE_NO_COLUMN;
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-row-active-handler")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return;
  }

  if (uitree->activeColumn != NULL) {
    if (uitree->activeColumn == column) {
      col = uitree->activecol;
    }
  }

  count = uiTreeViewSelectGetCount (uiwidget);
  uitree->selectset = false;
  if (count == 1 && uitree->selmode == SELECT_SINGLE) {
    bool  valid;

    valid = gtk_tree_selection_get_selected (uitree->sel, &uitree->model, &uitree->selectiter);
    if (valid) {
      uitree->selectset = true;
    }
  }
  if (uitree->callbacks [TV_CB_ROW_ACTIVE] != NULL) {
    callbackHandlerLong (uitree->callbacks [TV_CB_ROW_ACTIVE], col);
  }
}

static gboolean
uiTreeViewClickHandler (GtkWidget *tv, GdkEventButton *event, gpointer udata)
{
  uiwcont_t   *uiwidget = udata;
  int         count;
  uitree_t    *uitree;
  guint       button;

  /* other clicks are handled by the row-activated handler */
  /* and the select-changed handler */

  if (gdk_event_get_event_type ((GdkEvent *) event) != GDK_BUTTON_PRESS) {
    return UICB_CONT;
  }
  gdk_event_get_button ((GdkEvent *) event, &button);
  if (button != GDK_BUTTON_SECONDARY) {
    return UICB_CONT;
  }

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-click-handler")) {
    return UICB_STOP;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return UICB_STOP;
  }

  count = uiTreeViewSelectGetCount (uiwidget);
  uitree->selectset = false;
  if (count == 1 && uitree->selmode == SELECT_SINGLE) {
    bool  valid;

    valid = gtk_tree_selection_get_selected (uitree->sel, &uitree->model, &uitree->selectiter);
    if (valid) {
      uitree->selectset = true;
    }
  }
  if (uitree->callbacks [TV_CB_BUTTON_3] != NULL) {
    callbackHandler (uitree->callbacks [TV_CB_BUTTON_3]);
  }

  return UICB_CONT;
}

static gboolean
uiTreeViewForeachHandler (GtkTreeModel* model, GtkTreePath* path,
    GtkTreeIter* iter, gpointer udata)
{
  uiwcont_t   *uiwidget = udata;
  bool        rc = UI_FOREACH_CONT;
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-foreach-handler")) {
    return rc;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->model == NULL) {
    return rc;
  }

  memcpy (&uitree->selectiter, iter, sizeof (GtkTreeIter));
  uitree->selectset = true;
  if (uitree->callbacks [TV_CB_FOREACH] != NULL) {
    rc = callbackHandler (uitree->callbacks [TV_CB_FOREACH]);
  }

  return rc;
}

static GType
uiTreeViewConvertTreeType (int type)
{
  GType   gtktype = G_TYPE_LONG;

  switch (type) {
    case TREE_TYPE_STRING: {
      gtktype = G_TYPE_STRING;
      break;
    }
    case TREE_TYPE_NUM: {
      gtktype = G_TYPE_LONG;
      break;
    }
    case TREE_TYPE_ELLIPSIZE:
    case TREE_TYPE_INT: {
      gtktype = G_TYPE_INT;
      break;
    }
    case TREE_TYPE_WIDGET: {
      gtktype = G_TYPE_OBJECT;
      break;
    }
    case TREE_TYPE_BOOLEAN: {
      gtktype = G_TYPE_BOOLEAN;
      break;
    }
    case TREE_TYPE_IMAGE: {
      gtktype = GDK_TYPE_PIXBUF;
      break;
    }
    case TREE_TYPE_END: {
      fprintf (stderr, "ERR: tree-type-end found in conversion\n");
      break;
    }
    default: {
      fprintf (stderr, "ERR: unknown tree type: %d\n", type);
      break;
    }
  }

  return gtktype;
}

static void
uiTreeViewSelectForeachHandler (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, gpointer udata)
{
  uiwcont_t   *uiwidget = udata;
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-foreach-handler")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->selectprocessmode == SELECT_PROCESS_CLEAR_ALL) {
    gtk_tree_selection_unselect_iter (uitree->sel, iter);
  }

  if (uitree->selectprocessmode == SELECT_PROCESS_GET_SELECT_ITER) {
    memcpy (&uitree->selectiter, iter, sizeof (GtkTreeIter));
    uitree->selectset = true;
  }

  if (uitree->selectprocessmode == SELECT_PROCESS_NONE &&
      uitree->callbacks [TV_CB_FOREACH] != NULL) {
    char      *pathstr;
    int       row;

    pathstr = gtk_tree_path_to_string (path);
    mdextalloc (pathstr);
    row = atoi (pathstr);
    mdfree (pathstr);

    memcpy (&uitree->selectforeachiter, iter, sizeof (GtkTreeIter));

    callbackHandlerLong (uitree->callbacks [TV_CB_FOREACH], row);
  }
}

static void
uiTreeViewSelectChangedHandler (GtkTreeSelection *sel, gpointer udata)
{
  uiwcont_t   *uiwidget = udata;
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-select-chg-handler")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (uitree->callbacks [TV_CB_SELECT_CHG] != NULL) {
    callbackHandler (uitree->callbacks [TV_CB_SELECT_CHG]);
  }
}

static void
uiTreeViewSizeChangeHandler (GtkWidget* w, GtkAllocation* allocation,
    gpointer udata)
{
  uiwcont_t     *uiwidget = udata;
  GtkWidget     *tree;
  int           rows = 0;
  uitree_t      *uitree;
  bool          rc;
  GtkTreePath   *tpstart = NULL;
  GtkTreePath   *tpend = NULL;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-sz-chg-handler")) {
    return;
  }

  uitree = uiwidget->uiint.uitree;

  if (allocation->height == uitree->lastTreeSize) {
    return;
  }

  if (allocation->height < 200) {
    return;
  }

  tree = uiwidget->uidata.widget;
  rc = gtk_tree_view_get_visible_range (GTK_TREE_VIEW (tree), &tpstart, &tpend);
  if (rc) {
    char          *tstr;
    int           beg, end;

    mdextalloc (tpstart);
    mdextalloc (tpend);

    tstr = gtk_tree_path_to_string (tpstart);
    mdextalloc (tstr);
    beg = atoi (tstr);
    mdfree (tstr);
    tstr = gtk_tree_path_to_string (tpend);
    mdextalloc (tstr);
    end = atoi (tstr);
    mdfree (tstr);

    mdextfree (tpstart);
    gtk_tree_path_free (tpstart);
    mdextfree (tpend);
    gtk_tree_path_free (tpend);

    rows = end - beg + 1;
  }

  if (uitree->callbacks [TV_CB_SIZE_CHG] != NULL && rows > 0) {
    callbackHandlerLong (uitree->callbacks [TV_CB_SIZE_CHG], rows);
  }
}

static gboolean
uiTreeViewScrollEventHandler (GtkWidget* tv, GdkEventScroll *event,
    gpointer udata)
{
  uiwcont_t   *uiwidget = udata;
  int         dir = TREE_SCROLL_NEXT;
  int         rc = UICB_CONT;
  uitree_t    *uitree;

  if (! uiwcontValid (uiwidget, WCONT_T_TREE, "tree-scroll-event-handler")) {
    return rc;
  }

  uitree = uiwidget->uiint.uitree;

  /* i'd like to have a way to turn off smooth scrolling for the application */
  if (event->direction == GDK_SCROLL_SMOOTH) {
    double dx, dy;

    gdk_event_get_scroll_deltas ((GdkEvent *) event, &dx, &dy);
    if (dy < 0.0) {
      dir = TREE_SCROLL_PREV;
    }
    if (dy > 0.0) {
      dir = TREE_SCROLL_NEXT;
    }
  }
  if (event->direction == GDK_SCROLL_DOWN) {
    dir = TREE_SCROLL_NEXT;
  }
  if (event->direction == GDK_SCROLL_UP) {
    dir = TREE_SCROLL_PREV;
  }

  if (uitree->callbacks [TV_CB_SCROLL_EVENT] != NULL) {
    rc = callbackHandlerLong (uitree->callbacks [TV_CB_SCROLL_EVENT], dir);
  }

  return rc;
}
