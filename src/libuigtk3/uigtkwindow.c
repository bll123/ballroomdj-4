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
uiutilsCreateMainWindow (GApplication *app, char *title, char *imagenm,
    void *deletecb, void *udata)
{
  GtkWidget *window;
  GError    *gerr = NULL;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (window != NULL);
  gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (app));
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  if (imagenm != NULL) {
    gtk_window_set_default_icon_from_file (imagenm, &gerr);
  }
  gtk_window_set_title (GTK_WINDOW (window), title);
  g_signal_connect (window, "delete-event", G_CALLBACK (deletecb), udata);

  return window;
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
  gtk_widget_set_vexpand (widget, FALSE);
  twidget = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (widget));
  uiutilsSetCss (twidget,
      "scrollbar, scrollbar slider { min-width: 9px; } ");

  logProcEnd (LOG_PROC, "uiutilsCreateScrolledWindow", "");
  return widget;
}

