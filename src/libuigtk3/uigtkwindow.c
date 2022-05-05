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

#include "log.h"
#include "uiutils.h"

GtkWidget *
uiutilsCreateMainWindow (char *title, char *imagenm,
    void *deletecb, void *udata)
{
  GtkWidget *window;
  GError    *gerr = NULL;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (window != NULL);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  if (imagenm != NULL) {
    gtk_window_set_default_icon_from_file (imagenm, &gerr);
  }
  gtk_window_set_title (GTK_WINDOW (window), title);
  if (deletecb != NULL) {
    g_signal_connect (window, "delete-event", G_CALLBACK (deletecb), udata);
  }

  return window;
}

void
uiutilsCloseWindow (GtkWidget *window)
{
  if (GTK_IS_WIDGET (window)) {
    gtk_widget_destroy (window);
  }
}

inline bool
uiutilsWindowIsMaximized (GtkWidget *window)
{
  return (bool) gtk_window_is_maximized (GTK_WINDOW (window));
}

inline void
uiutilsWindowIconify (GtkWidget *window)
{
  gtk_window_iconify (GTK_WINDOW (window));
}

inline void
uiutilsWindowMaximize (GtkWidget *window)
{
  gtk_window_maximize (GTK_WINDOW (window));
}

inline void
uiutilsWindowUnMaximize (GtkWidget *window)
{
  gtk_window_unmaximize (GTK_WINDOW (window));
}

inline void
uiutilsWindowDisableDecorations (GtkWidget *window)
{
  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
}

inline void
uiutilsWindowEnableDecorations (GtkWidget *window)
{
  gtk_window_set_decorated (GTK_WINDOW (window), TRUE);
}

inline void
uiutilsWindowGetSize (GtkWidget *window, int *x, int *y)
{
  gtk_window_get_size (GTK_WINDOW (window), x, y);
}

inline void
uiutilsWindowSetDefaultSize (GtkWidget *window, int x, int y)
{
  gtk_window_set_default_size (GTK_WINDOW (window), x, y);
}

inline void
uiutilsWindowGetPosition (GtkWidget *window, int *x, int *y)
{
  gtk_window_get_position (GTK_WINDOW (window), x, y);
}

inline void
uiutilsWindowMove (GtkWidget *window, int x, int y)
{
  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (window), x, y);
  }
}

inline void
uiutilsWindowNoFocusOnStartup (GtkWidget *window)
{
  gtk_window_set_focus_on_map (GTK_WINDOW (window), FALSE);
}


GtkWidget *
uiutilsCreateScrolledWindow (void)
{
  GtkWidget   *widget;
  GtkWidget   *twidget;

  logProcBegin (LOG_PROC, "uiutilsCreateScrolledWindow");

  widget = gtk_scrolled_window_new (NULL, NULL);
  uiutilsSetCss (widget,
      "undershoot.top, undershoot.right, "
      "undershoot.left, undershoot.bottom "
      "{ background-image: none; outline-width: 0; }");
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (widget), FALSE);
  gtk_scrolled_window_set_kinetic_scrolling (GTK_SCROLLED_WINDOW (widget), FALSE);
  gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (widget), TRUE);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (widget), 400);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand (widget, TRUE);
//  gtk_widget_set_vexpand (widget, FALSE);
  gtk_widget_set_vexpand (widget, TRUE);
  twidget = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (widget));
  uiutilsSetCss (twidget,
      "scrollbar, scrollbar slider { min-width: 9px; } ");

  logProcEnd (LOG_PROC, "uiutilsCreateScrolledWindow", "");
  return widget;
}

