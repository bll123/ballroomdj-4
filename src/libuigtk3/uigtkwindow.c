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

#include "ui.h"
#include "uiutils.h"

static gboolean uiWindowCloseCallback (GtkWidget *window, GdkEvent *event, gpointer udata);
static gboolean uiWindowDoubleClickHandler (GtkWidget *window, GdkEventButton *event, gpointer udata);
static gboolean uiWindowWinStateHandler (GtkWidget *window, GdkEventWindowState *event, gpointer udata);
static void uiWindowNoDimHandler (GtkWidget *window, GtkStateType flags, gpointer udata);
static gboolean uiWindowMappedHandler (GtkWidget *window, GdkEventAny *event, gpointer udata);

void
uiCreateMainWindow (UIWidget *uiwidget, UICallback *uicb,
    const char *title, const char *imagenm)
{
  GtkWidget *window;
  GError    *gerr = NULL;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (window != NULL);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  if (imagenm != NULL) {
    gtk_window_set_icon_from_file (GTK_WINDOW (window), imagenm, &gerr);
  }
  if (title != NULL) {
    gtk_window_set_title (GTK_WINDOW (window), title);
  }
  if (uicb != NULL) {
    g_signal_connect (window, "delete-event", G_CALLBACK (uiWindowCloseCallback), uicb);
  }

  uiwidget->widget = window;
}

inline void
uiCloseWindow (UIWidget *uiwindow)
{
  if (GTK_IS_WIDGET (uiwindow->widget)) {
    gtk_widget_destroy (uiwindow->widget);
  }
}

inline bool
uiWindowIsMaximized (UIWidget *uiwindow)
{
  return (bool) gtk_window_is_maximized (GTK_WINDOW (uiwindow->widget));
}

inline void
uiWindowIconify (UIWidget *uiwindow)
{
  gtk_window_iconify (GTK_WINDOW (uiwindow->widget));
}

inline void
uiWindowMaximize (UIWidget *uiwindow)
{
  gtk_window_maximize (GTK_WINDOW (uiwindow->widget));
}

inline void
uiWindowUnMaximize (UIWidget *uiwindow)
{
  gtk_window_unmaximize (GTK_WINDOW (uiwindow->widget));
}

inline void
uiWindowDisableDecorations (UIWidget *uiwindow)
{
  gtk_window_set_decorated (GTK_WINDOW (uiwindow->widget), FALSE);
}

inline void
uiWindowEnableDecorations (UIWidget *uiwindow)
{
  gtk_window_set_decorated (GTK_WINDOW (uiwindow->widget), TRUE);
}

inline void
uiWindowGetSize (UIWidget *uiwindow, int *x, int *y)
{
  gtk_window_get_size (GTK_WINDOW (uiwindow->widget), x, y);
}

inline void
uiWindowSetDefaultSize (UIWidget *uiwindow, int x, int y)
{
  gtk_window_set_default_size (GTK_WINDOW (uiwindow->widget), x, y);
}

inline void
uiWindowGetPosition (UIWidget *uiwindow, int *x, int *y)
{
  gtk_window_get_position (GTK_WINDOW (uiwindow->widget), x, y);
}

inline void
uiWindowMove (UIWidget *uiwindow, int x, int y)
{
  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (uiwindow->widget), x, y);
  }
}

inline void
uiWindowNoFocusOnStartup (UIWidget *uiwindow)
{
  gtk_window_set_focus_on_map (GTK_WINDOW (uiwindow->widget), FALSE);
}


void
uiCreateScrolledWindow (UIWidget *uiwidget, int minheight)
{
  GtkWidget   *widget;
  widget = uiCreateScrolledWindowW (minheight);
  uiwidget->widget = widget;
}

void
uiWindowSetDoubleClickCallback (UIWidget *uiwidget, UICallback *uicb)
{
  g_signal_connect (uiwidget->widget, "button-press-event",
      G_CALLBACK (uiWindowDoubleClickHandler), uicb);
}

void
uiWindowSetWinStateCallback (UIWidget *uiwindow, UICallback *uicb)
{
  g_signal_connect (uiwindow->widget, "window-state-event",
      G_CALLBACK (uiWindowWinStateHandler), uicb);
}

void
uiWindowNoDim (UIWidget *uiwidget)
{
  g_signal_connect (uiwidget->widget, "state-flags-changed",
      G_CALLBACK (uiWindowNoDimHandler), uiwidget);
}

void
uiWindowSetMappedCallback (UIWidget *uiwidget, UICallback *uicb)
{
  g_signal_connect (uiwidget->widget, "map-event",
      G_CALLBACK (uiWindowMappedHandler), uicb);
}

/* internal routines */

static gboolean
uiWindowCloseCallback (GtkWidget *window, GdkEvent *event, gpointer udata)
{
  UICallback  *uicb = udata;
  bool        rc = false;

  rc = uiutilsCallbackHandler (uicb);
  return rc;
}

static gboolean
uiWindowDoubleClickHandler (GtkWidget *window, GdkEventButton *event, gpointer udata)
{
  UICallback  *uicb = udata;
  bool        rc;

  if (gdk_event_get_event_type ((GdkEvent *) event) != GDK_DOUBLE_BUTTON_PRESS) {
    return UICB_CONT;
  }

  rc = uiutilsCallbackHandler (uicb);
  return rc;
}

static gboolean
uiWindowWinStateHandler (GtkWidget *window, GdkEventWindowState *event, gpointer udata)
{
  UICallback  *uicb = udata;
  bool        rc;
  int         isicon = -1;
  int         ismax = -1;
  bool        process = false;

  if (uicb == NULL) {
    return UICB_CONT;
  }
  if (uicb->intintcb == NULL) {
    return UICB_CONT;
  }

  if (event->changed_mask == GDK_WINDOW_STATE_ICONIFIED) {
    isicon = false;
    if ((event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) ==
        GDK_WINDOW_STATE_ICONIFIED) {
      isicon = true;
    }
    process = true;
  }
  if (event->changed_mask == GDK_WINDOW_STATE_MAXIMIZED) {
    ismax = false;
    if ((event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) ==
        GDK_WINDOW_STATE_MAXIMIZED) {
      ismax = true;
    }
    process = true;
  }

  if (! process) {
    return UICB_CONT;
  }

  rc = uicb->intintcb (uicb->udata, isicon, ismax);
  return rc;
}

static gboolean
uiWindowMappedHandler (GtkWidget *window, GdkEventAny *event, gpointer udata)
{
  UICallback  *uicb = udata;
  bool        rc = false;

  rc = uiutilsCallbackHandler (uicb);
  return rc;
}


static void
uiWindowNoDimHandler (GtkWidget *window, GtkStateType flags, gpointer udata)
{
  /* never in a backdrop state */
  gtk_widget_unset_state_flags (GTK_WIDGET (window), GTK_STATE_FLAG_BACKDROP);
}

/* these routines will be removed at a later date */

GtkWidget *
uiCreateMainWindowW (const char *title, const char *imagenm,
    void *deletecb, void *udata)
{
  GtkWidget *window;
  GError    *gerr = NULL;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (window != NULL);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  if (imagenm != NULL) {
    gtk_window_set_icon_from_file (GTK_WINDOW (window), imagenm, &gerr);
  }
  gtk_window_set_title (GTK_WINDOW (window), title);
  if (deletecb != NULL) {
    g_signal_connect (window, "delete-event", G_CALLBACK (deletecb), udata);
  }

  return window;
}

inline void
uiCloseWindowW (GtkWidget *window)
{
  if (GTK_IS_WIDGET (window)) {
    gtk_widget_destroy (window);
  }
}

inline bool
uiWindowIsMaximizedW (GtkWidget *window)
{
  return (bool) gtk_window_is_maximized (GTK_WINDOW (window));
}

inline void
uiWindowIconifyW (GtkWidget *window)
{
  gtk_window_iconify (GTK_WINDOW (window));
}

inline void
uiWindowMaximizeW (GtkWidget *window)
{
  gtk_window_maximize (GTK_WINDOW (window));
}

inline void
uiWindowUnMaximizeW (GtkWidget *window)
{
  gtk_window_unmaximize (GTK_WINDOW (window));
}

inline void
uiWindowDisableDecorationsW (GtkWidget *window)
{
  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
}

inline void
uiWindowEnableDecorationsW (GtkWidget *window)
{
  gtk_window_set_decorated (GTK_WINDOW (window), TRUE);
}

inline void
uiWindowGetSizeW (GtkWidget *window, int *x, int *y)
{
  gtk_window_get_size (GTK_WINDOW (window), x, y);
}

inline void
uiWindowSetDefaultSizeW (GtkWidget *window, int x, int y)
{
  gtk_window_set_default_size (GTK_WINDOW (window), x, y);
}

inline void
uiWindowGetPositionW (GtkWidget *window, int *x, int *y)
{
  gtk_window_get_position (GTK_WINDOW (window), x, y);
}

inline void
uiWindowMoveW (GtkWidget *window, int x, int y)
{
  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (window), x, y);
  }
}

inline void
uiWindowNoFocusOnStartupW (GtkWidget *window)
{
  gtk_window_set_focus_on_map (GTK_WINDOW (window), FALSE);
}


GtkWidget *
uiCreateScrolledWindowW (int minheight)
{
  GtkWidget   *widget;
  GtkWidget   *twidget;

  widget = gtk_scrolled_window_new (NULL, NULL);
  uiSetCss (widget,
      "undershoot.top, undershoot.right, "
      "undershoot.left, undershoot.bottom "
      "{ background-image: none; outline-width: 0; }");
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (widget), FALSE);
  gtk_scrolled_window_set_kinetic_scrolling (GTK_SCROLLED_WINDOW (widget), FALSE);
  /* propagate natural width works well */
  gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (widget), TRUE);
  /* propagate natural height does not work, but set it anyways */
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (widget), TRUE);
  /* setting the min content height limits the scrolled window to that height */
  /* https://stackoverflow.com/questions/65449889/gtk-scrolledwindow-max-content-width-height-does-not-work-with-textview */
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (widget), minheight);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand (widget, FALSE);
  gtk_widget_set_vexpand (widget, FALSE);
  twidget = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (widget));
  uiSetCss (twidget,
      "scrollbar, scrollbar slider { min-width: 9px; } ");

  return widget;
}

