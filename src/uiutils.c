#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "ilist.h"
#include "log.h"
#include "pathbld.h"
#include "sysvars.h"
#include "uiutils.h"

static GLogWriterOutput uiutilsGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields, gsize n_fields, gpointer udata);

/* drop-down/combobox handling */
static void     uiutilsDropDownWindowShow (GtkButton *b, gpointer udata);
static gboolean uiutilsDropDownClose (GtkWidget *w,
    GdkEventFocus *event, gpointer udata);
static GtkWidget * uiutilsDropDownButtonCreate (char *title,
    uiutilsdropdown_t *dropdown);
static void uiutilsDropDownWindowCreate (uiutilsdropdown_t *dropdown,
    void *processSelectionCallback, void *udata);

void
uiutilsSetCss (GtkWidget *w, char *style)
{
  GtkCssProvider        *tcss;

  tcss = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (tcss, style, -1, NULL);
  gtk_style_context_add_provider (
      gtk_widget_get_style_context (GTK_WIDGET (w)),
      GTK_STYLE_PROVIDER (tcss),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void
uiutilsSetUIFont (char *uifont)
{
  GtkCssProvider  *tcss;
  GdkScreen       *screen;
  char            tbuff [MAXPATHLEN];
  char            wbuff [MAXPATHLEN];
  char            *p;
  int             sz = 0;

  strlcpy (wbuff, uifont, MAXPATHLEN);
  if (uifont != NULL && *uifont) {
    p = strrchr (wbuff, ' ');
    if (p != NULL) {
      ++p;
      if (isdigit (*p)) {
        --p;
        *p = '\0';
        ++p;
        sz = atoi (p);
      }
    }

    tcss = gtk_css_provider_new ();
    snprintf (tbuff, MAXPATHLEN, "* { font-family: '%s'; }", wbuff);
    if (sz > 0) {
      snprintf (wbuff, MAXPATHLEN, " * { font-size: %dpt; }", sz);
    }
    strlcat (tbuff, wbuff, MAXPATHLEN);
    gtk_css_provider_load_from_data (tcss, tbuff, -1, NULL);
    screen = gdk_screen_get_default ();
    if (screen != NULL) {
      gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (tcss),
          GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
  }
}

void
uiutilsInitGtkLog (void)
{
  g_log_set_writer_func (uiutilsGtkLogger, NULL, NULL);
}

GtkWidget *
uiutilsCreateColonLabel (char *label)
{
  GtkWidget *widget;
  char      tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "%s:", label);
  widget = gtk_label_new (tbuff);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);
  return widget;
}

GtkWidget *
uiutilsCreateButton (char *title, char *imagenm,
    void *clickCallback, void *udata)
{
  GtkWidget   *widget;

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);
  if (imagenm != NULL) {
    GtkWidget   *image;
    char        tbuff [MAXPATHLEN];

    gtk_button_set_label (GTK_BUTTON (widget), "");
    pathbldMakePath (tbuff, sizeof (tbuff), "", imagenm, ".svg",
        PATHBLD_MP_IMGDIR);
    image = gtk_image_new_from_file (tbuff);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE); // macos
    gtk_widget_set_tooltip_text (widget, title);
  } else {
    gtk_button_set_label (GTK_BUTTON (widget), title);
  }
  if (clickCallback != NULL) {
    g_signal_connect (widget, "clicked", G_CALLBACK (clickCallback), udata);
  }

  return widget;
}

GtkWidget *
uiutilsCreateScrolledWindow (void)
{
  GtkWidget   *widget;
  GtkWidget   *twidget;

  widget = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (widget), FALSE);
  gtk_scrolled_window_set_kinetic_scrolling (GTK_SCROLLED_WINDOW (widget), FALSE);
  gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (widget), 400);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_widget_set_vexpand (widget, FALSE);
  twidget = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (widget));
  uiutilsSetCss (twidget,
      "scrollbar, scrollbar slider { min-width: 9px; } ");

  return widget;
}

void
uiutilsDropDownInit (uiutilsdropdown_t *dropdown)
{
  dropdown->parentwin = NULL;
  dropdown->button = NULL;
  dropdown->window = NULL;
  dropdown->tree = NULL;
  dropdown->closeHandlerId = 0;
  dropdown->strSelection = NULL;
  dropdown->open = false;
  dropdown->iscombobox = false;
}

void
uiutilsDropDownFree (uiutilsdropdown_t *dropdown)
{
  if (dropdown->strSelection != NULL) {
    free (dropdown->strSelection);
  }
}

GtkWidget *
uiutilsDropDownCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uiutilsdropdown_t *dropdown, void *udata)
{
  dropdown->parentwin = parentwin;
  dropdown->button = uiutilsDropDownButtonCreate (title, dropdown);
  uiutilsDropDownWindowCreate (dropdown, processSelectionCallback, udata);
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

void
uiutilsCreateDanceList (uiutilsdropdown_t *dropdown)
{
  char              *dancenm;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];
  dance_t           *dances;
  ilist_t           *danceList;
  ilistidx_t        iteridx;
  ilistidx_t        idx;

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_STRING);

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceList = danceGetDanceList (dances);

  if (dropdown->iscombobox) {
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, -1,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, "", -1);
    gtk_list_store_append (store, &iter);
  }

  slistStartIterator (danceList, &iteridx);
  while ((dancenm = slistIterateKey (danceList, &iteridx)) != NULL) {
    idx = slistGetNum (danceList, dancenm);

    gtk_list_store_append (store, &iter);
    snprintf (tbuff, sizeof (tbuff), "%s    ", dancenm);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, idx,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff, -1);
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_DISP, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dropdown->tree), column);
  gtk_tree_view_set_model (GTK_TREE_VIEW (dropdown->tree),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

ssize_t
uiutilsDropDownSelectionGet (uiutilsdropdown_t *dropdown, GtkTreePath *path)
{
  GtkTreeIter   iter;
  GtkTreeModel  *model = NULL;
  unsigned long idx = 0;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dropdown->tree));
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_IDX, &idx, -1);
    if (dropdown->strSelection != NULL) {
      free (dropdown->strSelection);
    }
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_STR,
        &dropdown->strSelection, -1);
    gtk_widget_hide (dropdown->window);
    dropdown->open = false;
  } else {
    return -1;
  }

  return idx;
}

void
uiutilsDropDownSetList (uiutilsdropdown_t *dropdown, slist_t *list)
{
  char              *strval;
  char              *dispval;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];
  ilistidx_t        iteridx;
  gulong            idx;

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_STRING);

  slistStartIterator (list, &iteridx);

  idx = 0;
  while ((dispval = slistIterateKey (list, &iteridx)) != NULL) {
    strval = slistGetData (list, dispval);

    gtk_list_store_append (store, &iter);
    /* gtk does not leave room for the scrollbar */
    snprintf (tbuff, sizeof (tbuff), "%s    ", dispval);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, idx,
        UIUTILS_DROPDOWN_COL_STR, strval,
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        -1);
    ++idx;
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_DISP, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dropdown->tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (dropdown->tree),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

void
uiutilsDropDownSetNumList (uiutilsdropdown_t *dropdown, slist_t *list)
{
  char              *dispval;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];
  ilistidx_t        iteridx;
  gulong            idx;

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_STRING);

  slistStartIterator (list, &iteridx);

  while ((dispval = slistIterateKey (list, &iteridx)) != NULL) {
    idx = slistGetNum (list, dispval);

    gtk_list_store_append (store, &iter);
    /* gtk does not leave room for the scrollbar */
    snprintf (tbuff, sizeof (tbuff), "%s    ", dispval);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, idx,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        -1);
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_DISP, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dropdown->tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (dropdown->tree),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

void
uiutilsEntryInit (uiutilsentry_t *entry, int entrySize, int maxSize)
{
  entry->entrySize = entrySize;
  entry->maxSize = maxSize;
  entry->data = NULL;
  entry->buffer = NULL;
  entry->entry = NULL;
}

void
uiutilsEntryFree (uiutilsentry_t *entry)
{
  if (entry->data != NULL) {
    free (entry->data);
  }
}

GtkWidget *
uiutilsEntryCreate (uiutilsentry_t *entry)
{
  entry->data = malloc (entry->maxSize + 1);
  *(entry->data) = '\0';

  entry->buffer = gtk_entry_buffer_new (entry->data, -1);
  entry->entry = gtk_entry_new_with_buffer (entry->buffer);
  gtk_entry_set_width_chars (GTK_ENTRY (entry->entry), entry->entrySize);
  gtk_entry_set_max_length (GTK_ENTRY (entry->entry), entry->maxSize);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry->entry), GTK_INPUT_PURPOSE_FREE_FORM);
  gtk_widget_set_margin_top (entry->entry, 2);
  gtk_widget_set_margin_start (entry->entry, 2);
  gtk_widget_set_halign (entry->entry, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (entry->entry, TRUE);

  return entry->entry;
}

const char *
uiutilsEntryGetValue (uiutilsentry_t *entry)
{
  const char  *value;

  value = gtk_entry_buffer_get_text (entry->buffer);
  return value;
}

void
uiutilsEntrySetValue (uiutilsentry_t *entry, char *value)
{
  gtk_entry_buffer_set_text (entry->buffer, value, -1);
}

void
uiutilsSpinboxInit (uiutilsspinbox_t *spinbox)
{
  spinbox->spinbox = NULL;
}

void
uiutilsSpinboxFree (uiutilsspinbox_t *spinbox)
{
  ;
}

GtkWidget *
uiutilsSpinboxTextCreate (uiutilsspinbox_t *spinbox,
    ssize_t min, ssize_t max, void *displayCallback, void *udata)
{
  GtkWidget   *widget;

  widget = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(spinbutton), min, max);
  g_signal_connect (widget, "input", G_CALLBACK (displayCallback), udata);

  return widget;
}


/* internal routines */

static GLogWriterOutput
uiutilsGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields,
    gsize n_fields,
    gpointer udata)
{
  char      *msg;

  if (logLevel != G_LOG_LEVEL_DEBUG) {
    msg = g_log_writer_format_fields (logLevel, fields, n_fields, FALSE);
    logMsg (LOG_GTK, LOG_IMPORTANT, msg);
    if (strcmp (sysvarsGetStr (SV_BDJ4_RELEASELEVEL), "alpha") == 0) {
      fprintf (stderr, "%s\n", msg);
    }
  }

  return G_LOG_WRITER_HANDLED;
}

static void
uiutilsDropDownWindowShow (GtkButton *b, gpointer udata)
{
  uiutilsdropdown_t *dropdown = udata;
  GtkAllocation     alloc;
  gint              x, y;

  gtk_window_get_position (GTK_WINDOW (dropdown->parentwin), &x, &y);
  gtk_widget_get_allocation (GTK_WIDGET (dropdown->button), &alloc);
  gtk_widget_show_all (dropdown->window);
  gtk_window_move (GTK_WINDOW (dropdown->window), alloc.x + x + 4, alloc.y + y + 4 + 30);
  dropdown->open = true;
}

static gboolean
uiutilsDropDownClose (GtkWidget *w, GdkEventFocus *event, gpointer udata)
{
  uiutilsdropdown_t *dropdown = udata;

  if (dropdown->open) {
    gtk_widget_hide (GTK_WIDGET (dropdown->window));
    dropdown->open = false;
  }
  gtk_window_present (GTK_WINDOW (dropdown->parentwin));

  return FALSE;
}

static GtkWidget *
uiutilsDropDownButtonCreate (char *title, uiutilsdropdown_t *dropdown)
{
  GtkWidget   *widget;
  GtkWidget   *image;
  char        tbuff [MAXPATHLEN];

  widget = gtk_button_new ();
  assert (widget != NULL);
  strlcpy (tbuff, title, MAXPATHLEN);
  if (dropdown->iscombobox) {
    snprintf (tbuff, sizeof (tbuff), "- %s  ", title);
  }
  gtk_button_set_label (GTK_BUTTON (widget), tbuff);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_down_small", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_image_position (GTK_BUTTON (widget), GTK_POS_RIGHT);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (uiutilsDropDownWindowShow), dropdown);

  return widget;
}


static void
uiutilsDropDownWindowCreate (uiutilsdropdown_t *dropdown,
    void *processSelectionCallback, void *udata)
{
  GtkWidget         *scwin;
  GtkWidget         *twidget;
  GtkTreeSelection  *sel;

  dropdown->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_attached_to (GTK_WINDOW (dropdown->window), dropdown->button);
  gtk_window_set_transient_for (GTK_WINDOW (dropdown->window),
      GTK_WINDOW (dropdown->parentwin));
  gtk_window_set_decorated (GTK_WINDOW (dropdown->window), FALSE);
  gtk_window_set_deletable (GTK_WINDOW (dropdown->window), FALSE);
  gtk_window_set_type_hint (GTK_WINDOW (dropdown->window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dropdown->window), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (dropdown->window), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (dropdown->window), 4);
  gtk_widget_hide (GTK_WIDGET (dropdown->window));
  gtk_widget_set_events (GTK_WIDGET (dropdown->window), GDK_FOCUS_CHANGE_MASK);
  g_signal_connect (G_OBJECT (dropdown->window),
      "focus-out-event", G_CALLBACK (uiutilsDropDownClose), dropdown);

  scwin = uiutilsCreateScrolledWindow ();
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scwin), 300);
  gtk_scrolled_window_set_max_content_height (GTK_SCROLLED_WINDOW (scwin), 400);
  gtk_widget_set_hexpand (scwin, TRUE);
//  gtk_widget_set_vexpand (scwin, TRUE);
  twidget = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (scwin));
  uiutilsSetCss (twidget,
      "scrollbar, scrollbar slider { min-width: 9px; } ");
  gtk_container_add (GTK_CONTAINER (dropdown->window), scwin);

  dropdown->tree = gtk_tree_view_new ();
  assert (dropdown->tree != NULL);
  g_object_ref_sink (G_OBJECT (dropdown->tree));
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (dropdown->tree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dropdown->tree), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (dropdown->tree));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_hexpand (dropdown->tree, TRUE);
  gtk_widget_set_vexpand (dropdown->tree, TRUE);
  gtk_container_add (GTK_CONTAINER (scwin), dropdown->tree);
  g_signal_connect (dropdown->tree, "row-activated",
      G_CALLBACK (processSelectionCallback), udata);
}
