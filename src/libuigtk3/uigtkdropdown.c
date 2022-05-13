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

#include "bdj4.h"
#include "bdjstring.h"
#include "pathbld.h"
#include "ui.h"
#include "uiutils.h"

/* drop-down/combobox handling */
static void     uiDropDownWindowShow (GtkButton *b, gpointer udata);
static gboolean uiDropDownClose (GtkWidget *w,
    GdkEventFocus *event, gpointer udata);
static GtkWidget * uiDropDownButtonCreate (uidropdown_t *dropdown);
static void uiDropDownWindowCreate (uidropdown_t *dropdown,
    void *processSelectionCallback, void *udata);
static void uiDropDownSelectionSet (uidropdown_t *dropdown,
    nlistidx_t internalidx);

void
uiDropDownInit (uidropdown_t *dropdown)
{
  dropdown->title = NULL;
  dropdown->parentwin = NULL;
  dropdown->button = NULL;
  dropdown->window = NULL;
  dropdown->tree = NULL;
  dropdown->sel = NULL;
  dropdown->closeHandlerId = 0;
  dropdown->strSelection = NULL;
  dropdown->strIndexMap = NULL;
  dropdown->keylist = NULL;
  dropdown->open = false;
  dropdown->iscombobox = false;
}


void
uiDropDownFree (uidropdown_t *dropdown)
{
  if (dropdown != NULL) {
    if (dropdown->title != NULL) {
      free (dropdown->title);
    }
    if (dropdown->strSelection != NULL) {
      free (dropdown->strSelection);
    }
    if (dropdown->strIndexMap != NULL) {
      slistFree (dropdown->strIndexMap);
    }
    if (dropdown->keylist != NULL) {
      nlistFree (dropdown->keylist);
    }
  }
}

GtkWidget *
uiDropDownCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uidropdown_t *dropdown, void *udata)
{
  dropdown->parentwin = parentwin;
  dropdown->title = strdup (title);
  dropdown->button = uiDropDownButtonCreate (dropdown);
  uiDropDownWindowCreate (dropdown, processSelectionCallback, udata);
  return dropdown->button;
}

GtkWidget *
uiComboboxCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uidropdown_t *dropdown, void *udata)
{
  dropdown->iscombobox = true;
  return uiDropDownCreate (parentwin,
      title, processSelectionCallback,
      dropdown, udata);
}

nlistidx_t
uiDropDownSelectionGet (uidropdown_t *dropdown, GtkTreePath *path)
{
  GtkTreeIter   iter;
  GtkTreeModel  *model = NULL;
  gulong        idx = 0;
  int32_t       idx32;
  nlistidx_t    retval;
  char          tbuff [100];

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dropdown->tree));
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_IDX, &idx, -1);
    /* despite the model using an unsigned long, setting it to -1 */
    /* sets it to a 32-bit value */
    /* want the special -1 index (all items for combobox) */
    idx32 = (int32_t) idx;
    retval = idx32;
    if (dropdown->strSelection != NULL) {
      free (dropdown->strSelection);
    }
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_STR,
        &dropdown->strSelection, -1);
    if (dropdown->iscombobox) {
      char  *p;

      gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_DISP, &p, -1);
      snprintf (tbuff, sizeof (tbuff), "%-*s", dropdown->maxwidth, p);
      gtk_button_set_label (GTK_BUTTON (dropdown->button), p);
      free (p);
    }
    gtk_widget_hide (dropdown->window);
    dropdown->open = false;
  } else {
    return -1;
  }

  return retval;
}

void
uiDropDownSetList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel)
{
  char              *strval;
  char              *dispval;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];
  ilistidx_t        iteridx;
  nlistidx_t        internalidx;


  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  dropdown->strIndexMap = slistAlloc ("uiutils-str-index", LIST_ORDERED, NULL);
  internalidx = 0;

  dropdown->maxwidth = slistGetMaxKeyWidth (list);

  if (! dropdown->iscombobox) {
    gtk_button_set_label (GTK_BUTTON (dropdown->button), dropdown->title);
  }

  if (dropdown->iscombobox && selectLabel != NULL) {
    snprintf (tbuff, sizeof (tbuff), "%-*s",
        dropdown->maxwidth, selectLabel);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, (gulong) -1,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        UIUTILS_DROPDOWN_COL_SB_PAD, "      ",
        -1);
    slistSetNum (dropdown->strIndexMap, "", internalidx++);
  }

  slistStartIterator (list, &iteridx);

  while ((dispval = slistIterateKey (list, &iteridx)) != NULL) {
    strval = slistGetStr (list, dispval);
    slistSetNum (dropdown->strIndexMap, strval, internalidx);

    gtk_list_store_append (store, &iter);
    snprintf (tbuff, sizeof (tbuff), "%-*s",
        dropdown->maxwidth, dispval);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, (gulong) internalidx,
        UIUTILS_DROPDOWN_COL_STR, strval,
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        UIUTILS_DROPDOWN_COL_SB_PAD, "      ",
        -1);
    ++internalidx;
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_DISP, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dropdown->tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_SB_PAD, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dropdown->tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (dropdown->tree),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

void
uiDropDownSetNumList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel)
{
  char              *dispval;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];
  ilistidx_t        iteridx;
  nlistidx_t        internalidx;
  nlistidx_t        idx;

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  dropdown->keylist = nlistAlloc ("uiutils-keylist", LIST_ORDERED, NULL);
  internalidx = 0;

  dropdown->maxwidth = slistGetMaxKeyWidth (list);

  if (! dropdown->iscombobox) {
    gtk_button_set_label (GTK_BUTTON (dropdown->button), dropdown->title);
  }

  if (dropdown->iscombobox && selectLabel != NULL) {
    snprintf (tbuff, sizeof (tbuff), "%-*s",
        dropdown->maxwidth, selectLabel);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, (gulong) -1,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        UIUTILS_DROPDOWN_COL_SB_PAD, "    ",
        -1);
    nlistSetNum (dropdown->keylist, -1, internalidx);
    ++internalidx;
  }

  slistStartIterator (list, &iteridx);

  while ((dispval = slistIterateKey (list, &iteridx)) != NULL) {
    idx = slistGetNum (list, dispval);
    nlistSetNum (dropdown->keylist, idx, internalidx);

    gtk_list_store_append (store, &iter);
    snprintf (tbuff, sizeof (tbuff), "%-*s",
        slistGetMaxKeyWidth (list), dispval);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, (gulong) idx,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        UIUTILS_DROPDOWN_COL_SB_PAD, "    ",
        -1);
    ++internalidx;
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_DISP, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dropdown->tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_SB_PAD, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dropdown->tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (dropdown->tree),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

void
uiDropDownSelectionSetNum (uidropdown_t *dropdown, nlistidx_t idx)
{
  nlistidx_t    internalidx;


  if (dropdown == NULL) {
    return;
  }

  if (dropdown->keylist == NULL) {
    internalidx = 0;
  } else {
    internalidx = nlistGetNum (dropdown->keylist, idx);
  }
  uiDropDownSelectionSet (dropdown, internalidx);
}

void
uiDropDownSelectionSetStr (uidropdown_t *dropdown, char *stridx)
{
  nlistidx_t    internalidx;


  if (dropdown == NULL) {
    return;
  }

  if (dropdown->strIndexMap == NULL) {
    internalidx = 0;
  } else {
    internalidx = slistGetNum (dropdown->strIndexMap, stridx);
    if (internalidx < 0) {
      internalidx = 0;
    }
  }
  uiDropDownSelectionSet (dropdown, internalidx);
}

/* internal routines */

static void
uiDropDownWindowShow (GtkButton *b, gpointer udata)
{
  uidropdown_t *dropdown = udata;
  GtkAllocation     alloc;
  gint              x, y;


  gtk_window_get_position (GTK_WINDOW (dropdown->parentwin), &x, &y);
  gtk_widget_get_allocation (dropdown->button, &alloc);
  gtk_widget_show_all (dropdown->window);
  gtk_window_move (GTK_WINDOW (dropdown->window), alloc.x + x + 4, alloc.y + y + 4 + 30);
  dropdown->open = true;
}

static gboolean
uiDropDownClose (GtkWidget *w, GdkEventFocus *event, gpointer udata)
{
  uidropdown_t *dropdown = udata;


  if (dropdown->open) {
    gtk_widget_hide (dropdown->window);
    dropdown->open = false;
  }
  gtk_window_present (GTK_WINDOW (dropdown->parentwin));

  return FALSE;
}

static GtkWidget *
uiDropDownButtonCreate (uidropdown_t *dropdown)
{
  GtkWidget   *widget;
  GtkWidget   *image;
  char        tbuff [MAXPATHLEN];


  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_down_small", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_image_position (GTK_BUTTON (widget), GTK_POS_RIGHT);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);

  g_signal_connect (widget, "clicked",
      G_CALLBACK (uiDropDownWindowShow), dropdown);

  return widget;
}


static void
uiDropDownWindowCreate (uidropdown_t *dropdown,
    void *processSelectionCallback, void *udata)
{
  GtkWidget         *scwin;
  GtkWidget         *vbox;
  GtkWidget         *twidget;


  dropdown->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_attached_to (GTK_WINDOW (dropdown->window), dropdown->button);
  gtk_window_set_transient_for (GTK_WINDOW (dropdown->window),
      GTK_WINDOW (dropdown->parentwin));
  gtk_window_set_decorated (GTK_WINDOW (dropdown->window), FALSE);
  gtk_window_set_deletable (GTK_WINDOW (dropdown->window), FALSE);
  gtk_window_set_type_hint (GTK_WINDOW (dropdown->window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dropdown->window), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (dropdown->window), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (dropdown->window), 6);
  gtk_widget_hide (dropdown->window);
  gtk_widget_set_vexpand (dropdown->window, FALSE);
  gtk_widget_set_events (dropdown->window, GDK_FOCUS_CHANGE_MASK);
  g_signal_connect (G_OBJECT (dropdown->window),
      "focus-out-event", G_CALLBACK (uiDropDownClose), dropdown);

  twidget = uiCreateVertBoxWW ();
  gtk_widget_set_hexpand (twidget, TRUE);
  gtk_widget_set_vexpand (twidget, TRUE);
  gtk_container_add (GTK_CONTAINER (dropdown->window), twidget);

  vbox = uiCreateVertBoxWW ();
  uiBoxPackStartExpandWW (twidget, vbox);

  scwin = uiCreateScrolledWindow (300);
  gtk_widget_set_hexpand (scwin, TRUE);
  gtk_widget_set_vexpand (scwin, FALSE);
  twidget = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (scwin));
  uiSetCss (twidget,
      "scrollbar, scrollbar slider { min-width: 9px; } ");
  uiBoxPackStartExpandWW (vbox, scwin);

  dropdown->tree = gtk_tree_view_new ();
  assert (dropdown->tree != NULL);
  if (G_IS_OBJECT (dropdown->tree)) {
    g_object_ref_sink (G_OBJECT (dropdown->tree));
  }
  dropdown->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (dropdown->tree));
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (dropdown->tree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dropdown->tree), FALSE);
  gtk_tree_selection_set_mode (dropdown->sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_hexpand (dropdown->tree, TRUE);
  gtk_widget_set_vexpand (dropdown->tree, TRUE);
  gtk_container_add (GTK_CONTAINER (scwin), dropdown->tree);
  g_signal_connect (dropdown->tree, "row-activated",
      G_CALLBACK (processSelectionCallback), udata);
}

static void
uiDropDownSelectionSet (uidropdown_t *dropdown, nlistidx_t internalidx)
{
  GtkTreePath   *path;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  char          tbuff [100];
  char          *p;


  if (dropdown == NULL) {
    return;
  }
  if (dropdown->tree == NULL) {
    return;
  }

  if (internalidx < 0) {
    internalidx = 0;
  }
  snprintf (tbuff, sizeof (tbuff), "%d", internalidx);
  path = gtk_tree_path_new_from_string (tbuff);
  if (path != NULL) {
    gtk_tree_selection_select_path (dropdown->sel, path);
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (dropdown->tree));
    if (model != NULL) {
      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_DISP, &p, -1);
      if (p != NULL) {
        snprintf (tbuff, sizeof (tbuff), "%-*s", dropdown->maxwidth, p);
        gtk_button_set_label (GTK_BUTTON (dropdown->button), tbuff);
      }
    }
  }
}

