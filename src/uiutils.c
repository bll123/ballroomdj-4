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
static gboolean uiutilsCloseDanceSel (GtkWidget *w,
    GdkEventFocus *event, gpointer udata);

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
uiutilsCreateButton (char *title, char *imagenm,
    void *clickCallback, void *udata)
{
  GtkWidget   *widget;

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
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
uiutilsCreateDropDownButton (char *title, void *clickCallback, void *udata)
{
  GtkWidget   *widget;
  GtkWidget   *image;
  char        tbuff [MAXPATHLEN];

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), title);
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_down_small", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_image_position (GTK_BUTTON (widget), GTK_POS_RIGHT);
  g_signal_connect (widget, "clicked", G_CALLBACK (clickCallback), udata);

  return widget;
}

void
uiutilsCreateDropDown (GtkWidget *parentwin, GtkWidget *parentwidget,
    void *closeCallback, void *processSelectionCallback,
    GtkWidget **win, GtkWidget **treeview, void *closeudata, void *udata)
{
  GtkWidget         *scwin;
  GtkWidget         *twidget;
  GtkTreeSelection  *sel;


  *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_attached_to (GTK_WINDOW (*win), parentwidget);
  gtk_window_set_transient_for (GTK_WINDOW (*win),
      GTK_WINDOW (parentwin));
  gtk_window_set_decorated (GTK_WINDOW (*win), FALSE);
  gtk_window_set_deletable (GTK_WINDOW (*win), FALSE);
  gtk_window_set_type_hint (GTK_WINDOW (*win), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (*win), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (*win), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (*win), 4);
  gtk_widget_hide (GTK_WIDGET (*win));
  gtk_widget_set_events (GTK_WIDGET (*win), GDK_FOCUS_CHANGE_MASK);
  g_signal_connect (G_OBJECT (*win),
      "focus-out-event", G_CALLBACK (closeCallback), closeudata);

  scwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (scwin), FALSE);
  gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (scwin), TRUE);
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (scwin), TRUE);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scwin), 300);
  gtk_scrolled_window_set_max_content_height (GTK_SCROLLED_WINDOW (scwin), 400);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand (scwin, TRUE);
  gtk_widget_set_vexpand (scwin, TRUE);
  twidget = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (scwin));
  uiutilsSetCss (twidget,
      "scrollbar, scrollbar slider { min-width: 9px; } ");
  gtk_container_add (GTK_CONTAINER (*win), scwin);

  *treeview = gtk_tree_view_new ();
  assert (*treeview != NULL);
  g_object_ref_sink (G_OBJECT (*treeview));
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (*treeview), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (*treeview), FALSE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (*treeview));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_hexpand (*treeview, TRUE);
  gtk_widget_set_vexpand (*treeview, TRUE);
  gtk_container_add (GTK_CONTAINER (scwin), *treeview);
  g_signal_connect (*treeview, "row-activated",
      G_CALLBACK (processSelectionCallback), udata);
}

void
uiutilsCreateDanceSelect (GtkWidget *parentwin,
    char *label, uiutilsdancesel_t *dancesel,
    void *processSelectionCallback, void *udata)
{
  dancesel->danceSelectButton = uiutilsCreateDropDownButton (label,
      uiutilsShowDanceWindow, dancesel);
  uiutilsCreateDropDown (parentwin, dancesel->danceSelectButton,
      uiutilsCloseDanceSel, processSelectionCallback,
      &dancesel->danceSelectWin, &dancesel->danceSelect,
      dancesel, udata);
  dancesel->parentwin = parentwin;
  uiutilsCreateDanceList (dancesel);
}

void
uiutilsShowDanceWindow (GtkButton *b, gpointer udata)
{
  uiutilsdancesel_t *dancesel = udata;
  GtkAllocation     alloc;
  gint              x, y;

  gtk_window_get_position (GTK_WINDOW (dancesel->parentwin), &x, &y);
  gtk_widget_get_allocation (GTK_WIDGET (dancesel->danceSelectButton), &alloc);
  gtk_widget_show_all (dancesel->danceSelectWin);
  gtk_window_move (GTK_WINDOW (dancesel->danceSelectWin), alloc.x + x + 4, alloc.y + y + 4 + 30);
  dancesel->danceSelectOpen = true;
}

void
uiutilsCreateDanceList (uiutilsdancesel_t *dancesel)
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

  store = gtk_list_store_new (UIUTILS_DANCE_COL_MAX, G_TYPE_ULONG, G_TYPE_STRING);

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceList = danceGetDanceList (dances);

  slistStartIterator (danceList, &iteridx);
  while ((dancenm = slistIterateKey (danceList, &iteridx)) != NULL) {
    idx = slistGetNum (danceList, dancenm);

    gtk_list_store_append (store, &iter);
    snprintf (tbuff, sizeof (tbuff), "%s    ", dancenm);
    gtk_list_store_set (store, &iter,
        UIUTILS_DANCE_COL_IDX, idx,
        UIUTILS_DANCE_COL_NAME, tbuff, -1);
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DANCE_COL_NAME, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dancesel->danceSelect), column);
  gtk_tree_view_set_model (GTK_TREE_VIEW (dancesel->danceSelect),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

ssize_t
uiutilsGetDanceSelection (uiutilsdancesel_t *dancesel, GtkTreePath *path)
{
  GtkTreeIter   iter;
  GtkTreeModel  *model = NULL;
  unsigned long idx = 0;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dancesel->danceSelect));
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, UIUTILS_DANCE_COL_IDX, &idx, -1);
    gtk_widget_hide (dancesel->danceSelectWin);
    dancesel->danceSelectOpen = false;
  } else {
    return -1;
  }

  return idx;
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

static gboolean
uiutilsCloseDanceSel (GtkWidget *w, GdkEventFocus *event, gpointer udata)
{
  uiutilsdancesel_t *dancesel = udata;

  if (dancesel->danceSelectOpen) {
    gtk_widget_hide (GTK_WIDGET (dancesel->danceSelectWin));
    dancesel->danceSelectOpen = false;
  }
  gtk_window_present (GTK_WINDOW (dancesel->parentwin));

  return FALSE;
}

