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
#include "log.h"
#include "pathbld.h"
#include "uiutils.h"

/* drop-down/combobox handling */
static void     uiutilsDropDownWindowShow (GtkButton *b, gpointer udata);
static gboolean uiutilsDropDownClose (GtkWidget *w,
    GdkEventFocus *event, gpointer udata);
static GtkWidget * uiutilsDropDownButtonCreate (char *title,
    uiutilsdropdown_t *dropdown);
static void uiutilsDropDownWindowCreate (uiutilsdropdown_t *dropdown,
    void *processSelectionCallback, void *udata);
static void uiutilsDropDownSelectionSet (uiutilsdropdown_t *dropdown,
    ssize_t internalidx);

void
uiutilsDropDownInit (uiutilsdropdown_t *dropdown)
{
  logProcBegin (LOG_PROC, "uiutilsDropDownInit");
  dropdown->parentwin = NULL;
  dropdown->button = NULL;
  dropdown->window = NULL;
  dropdown->tree = NULL;
  dropdown->closeHandlerId = 0;
  dropdown->strSelection = NULL;
  dropdown->strIndexMap = NULL;
  dropdown->numIndexMap = NULL;
  dropdown->open = false;
  dropdown->iscombobox = false;
  logProcEnd (LOG_PROC, "uiutilsDropDownInit", "");
}


void
uiutilsDropDownFree (uiutilsdropdown_t *dropdown)
{
  logProcBegin (LOG_PROC, "uiutilsDropDownFree");
  if (dropdown != NULL) {
    if (dropdown->strSelection != NULL) {
      free (dropdown->strSelection);
    }
    if (dropdown->strIndexMap != NULL) {
      slistFree (dropdown->strIndexMap);
    }
    if (dropdown->numIndexMap != NULL) {
      nlistFree (dropdown->numIndexMap);
    }
  }
  logProcEnd (LOG_PROC, "uiutilsDropDownFree", "");
}

GtkWidget *
uiutilsDropDownCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uiutilsdropdown_t *dropdown, void *udata)
{
  logProcBegin (LOG_PROC, "uiutilsDropDownCreate");
  dropdown->parentwin = parentwin;
  dropdown->button = uiutilsDropDownButtonCreate (title, dropdown);
  uiutilsDropDownWindowCreate (dropdown, processSelectionCallback, udata);
  logProcEnd (LOG_PROC, "uiutilsDropDownCreate", "");
  return dropdown->button;
}

GtkWidget *
uiutilsComboboxCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uiutilsdropdown_t *dropdown, void *udata)
{
  dropdown->iscombobox = true;
  return uiutilsDropDownCreate (parentwin,
      title, processSelectionCallback,
      dropdown, udata);
}

ssize_t
uiutilsDropDownSelectionGet (uiutilsdropdown_t *dropdown, GtkTreePath *path)
{
  GtkTreeIter   iter;
  GtkTreeModel  *model = NULL;
  gulong        idx = 0;
  int32_t       idx32;

  logProcBegin (LOG_PROC, "uiutilsDropDownSelectionGet");

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dropdown->tree));
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_IDX, &idx, -1);
    /* despite the model using an unsigned long, setting it to -1 */
    /* sets it to a 32-bit value */
    /* want the special -1 index (all items for combobox) */
    idx32 = (int32_t) idx;
    if (dropdown->strSelection != NULL) {
      free (dropdown->strSelection);
    }
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_STR,
        &dropdown->strSelection, -1);
    if (dropdown->iscombobox) {
      char  *p;

      gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_DISP, &p, -1);
      gtk_button_set_label (GTK_BUTTON (dropdown->button), p);
      free (p);
    }
    gtk_widget_hide (dropdown->window);
    dropdown->open = false;
  } else {
    logProcEnd (LOG_PROC, "uiutilsDropDownSelectionGet", "no-iter");
    return -1;
  }

  logProcEnd (LOG_PROC, "uiutilsDropDownSelectionGet", "");
  return (ssize_t) idx32;
}

void
uiutilsDropDownSetList (uiutilsdropdown_t *dropdown, slist_t *list,
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
  ssize_t           internalidx;

  logProcBegin (LOG_PROC, "uiutilsDropDownSetList");

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  dropdown->strIndexMap = slistAlloc ("uiutils-str-index", LIST_ORDERED, NULL);
  internalidx = 0;

  if (dropdown->iscombobox && selectLabel != NULL) {
    snprintf (tbuff, sizeof (tbuff), "%-*s",
        slistGetMaxKeyWidth (list), selectLabel);
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
        slistGetMaxKeyWidth (list), dispval);
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
  logProcEnd (LOG_PROC, "uiutilsDropDownSetList", "");
}

void
uiutilsDropDownSetNumList (uiutilsdropdown_t *dropdown, slist_t *list,
    const char *selectLabel)
{
  char              *dispval;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];
  ilistidx_t        iteridx;
  ssize_t            internalidx;
  nlistidx_t        idx;

  logProcBegin (LOG_PROC, "uiutilsDropDownSetNumList");

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  dropdown->numIndexMap = nlistAlloc ("uiutils-num-index", LIST_ORDERED, NULL);
  internalidx = 0;

  if (dropdown->iscombobox && selectLabel != NULL) {
    snprintf (tbuff, sizeof (tbuff), "%-*s",
        slistGetMaxKeyWidth (list), selectLabel);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, (gulong) -1,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        UIUTILS_DROPDOWN_COL_SB_PAD, "    ",
        -1);
    nlistSetNum (dropdown->numIndexMap, -1, internalidx++);
  }

  slistStartIterator (list, &iteridx);

  while ((dispval = slistIterateKey (list, &iteridx)) != NULL) {
    idx = slistGetNum (list, dispval);
    nlistSetNum (dropdown->numIndexMap, idx, internalidx);

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
  logProcEnd (LOG_PROC, "uiutilsDropDownSetNumList", "");
}

void
uiutilsDropDownSelectionSetNum (uiutilsdropdown_t *dropdown, nlistidx_t idx)
{
  ssize_t        internalidx;

  logProcBegin (LOG_PROC, "uiutilsDropDownSelectionSetNum");

  if (dropdown == NULL) {
    logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSetNum", "null-dropdown");
    return;
  }

  if (dropdown->numIndexMap == NULL) {
    internalidx = 0;
  } else {
    internalidx = nlistGetNum (dropdown->numIndexMap, idx);
  }
  uiutilsDropDownSelectionSet (dropdown, internalidx);
  logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSetNum", "");
}

void
uiutilsDropDownSelectionSetStr (uiutilsdropdown_t *dropdown, char *stridx)
{
  ssize_t        internalidx;

  logProcBegin (LOG_PROC, "uiutilsDropDownSelectionSetStr");

  if (dropdown == NULL) {
    logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSetStr", "null-dropdown");
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
  uiutilsDropDownSelectionSet (dropdown, internalidx);
  logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSetStr", "");
}

/* internal routines */

static void
uiutilsDropDownWindowShow (GtkButton *b, gpointer udata)
{
  uiutilsdropdown_t *dropdown = udata;
  GtkAllocation     alloc;
  gint              x, y;

  logProcBegin (LOG_PROC, "uiutilsDropDownWindowShow");

  gtk_window_get_position (GTK_WINDOW (dropdown->parentwin), &x, &y);
  gtk_widget_get_allocation (dropdown->button, &alloc);
  gtk_widget_show_all (dropdown->window);
  gtk_window_move (GTK_WINDOW (dropdown->window), alloc.x + x + 4, alloc.y + y + 4 + 30);
  dropdown->open = true;
  logProcEnd (LOG_PROC, "uiutilsDropDownWindowShow", "");
}

static gboolean
uiutilsDropDownClose (GtkWidget *w, GdkEventFocus *event, gpointer udata)
{
  uiutilsdropdown_t *dropdown = udata;

  logProcBegin (LOG_PROC, "uiutilsDropDownClose");

  if (dropdown->open) {
    gtk_widget_hide (dropdown->window);
    dropdown->open = false;
  }
  gtk_window_present (GTK_WINDOW (dropdown->parentwin));

  logProcEnd (LOG_PROC, "uiutilsDropDownClose", "");
  return FALSE;
}

static GtkWidget *
uiutilsDropDownButtonCreate (char *title, uiutilsdropdown_t *dropdown)
{
  GtkWidget   *widget;
  GtkWidget   *image;
  char        tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "uiutilsDropDownButtonCreate");

  widget = gtk_button_new ();
  assert (widget != NULL);
  strlcpy (tbuff, title, MAXPATHLEN);
  if (dropdown->iscombobox) {
    snprintf (tbuff, sizeof (tbuff), "- %s  ", title);
  }
  gtk_button_set_label (GTK_BUTTON (widget), tbuff);
  gtk_widget_set_margin_top (widget, uiutilsBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiutilsBaseMarginSz);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_down_small", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_image_position (GTK_BUTTON (widget), GTK_POS_RIGHT);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);

  g_signal_connect (widget, "clicked",
      G_CALLBACK (uiutilsDropDownWindowShow), dropdown);

  logProcEnd (LOG_PROC, "uiutilsDropDownButtonCreate", "");
  return widget;
}


static void
uiutilsDropDownWindowCreate (uiutilsdropdown_t *dropdown,
    void *processSelectionCallback, void *udata)
{
  GtkWidget         *scwin;
  GtkWidget         *twidget;
  GtkTreeSelection  *sel;

  logProcBegin (LOG_PROC, "uiutilsDropDownWindowCreate");

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
      "focus-out-event", G_CALLBACK (uiutilsDropDownClose), dropdown);

  scwin = uiutilsCreateScrolledWindow ();
  gtk_scrolled_window_set_max_content_height (GTK_SCROLLED_WINDOW (scwin), 400);
  /* setting the min content height limits the scrolled window to that height */
  /* https://stackoverflow.com/questions/65449889/gtk-scrolledwindow-max-content-width-height-does-not-work-with-textview */
  gtk_widget_set_hexpand (scwin, TRUE);
  gtk_widget_set_vexpand (scwin, FALSE);
  twidget = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (scwin));
  uiutilsSetCss (twidget,
      "scrollbar, scrollbar slider { min-width: 9px; } ");
  gtk_container_add (GTK_CONTAINER (dropdown->window), scwin);

  dropdown->tree = gtk_tree_view_new ();
  assert (dropdown->tree != NULL);
  if (G_IS_OBJECT (dropdown->tree)) {
    g_object_ref_sink (G_OBJECT (dropdown->tree));
  }
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (dropdown->tree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dropdown->tree), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (dropdown->tree));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_hexpand (dropdown->tree, TRUE);
  gtk_widget_set_vexpand (dropdown->tree, TRUE);
  gtk_container_add (GTK_CONTAINER (scwin), dropdown->tree);
  g_signal_connect (dropdown->tree, "row-activated",
      G_CALLBACK (processSelectionCallback), udata);
  logProcEnd (LOG_PROC, "uiutilsDropDownWindowCreate", "");
}

static void
uiutilsDropDownSelectionSet (uiutilsdropdown_t *dropdown, ssize_t internalidx)
{
  GtkTreePath   *path;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  char          tbuff [40];
  char          *p;

  logProcBegin (LOG_PROC, "uiutilsDropDownSelectionSet");

  if (dropdown == NULL) {
    logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSet", "null-dropdown");
    return;
  }
  if (dropdown->tree == NULL) {
    logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSet", "null-tree");
    return;
  }

  if (internalidx < 0) {
    internalidx = 0;
  }
  snprintf (tbuff, sizeof (tbuff), "%zd", internalidx);
  path = gtk_tree_path_new_from_string (tbuff);
  if (path != NULL) {
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (dropdown->tree), path, NULL, FALSE);
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (dropdown->tree));
    if (model != NULL) {
      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_DISP, &p, -1);
      if (p != NULL) {
        gtk_button_set_label (GTK_BUTTON (dropdown->button), p);
      }
    }
  }
  logProcEnd (LOG_PROC, "uiutilsDropDownSelectionSet", "");
}

