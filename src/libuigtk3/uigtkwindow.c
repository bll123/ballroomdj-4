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
#if _hdr_gdk_gdkx
# include <gdk/gdkx.h>
#endif

#include "ui.h"
#include "callback.h"

static gboolean uiWindowFocusOutCallback (GtkWidget *w, GdkEventFocus *event, gpointer udata);
static gboolean uiWindowCloseCallback (GtkWidget *window, GdkEvent *event, gpointer udata);
static gboolean uiWindowDoubleClickHandler (GtkWidget *window, GdkEventButton *event, gpointer udata);
static gboolean uiWindowWinStateHandler (GtkWidget *window, GdkEventWindowState *event, gpointer udata);
static void uiWindowNoDimHandler (GtkWidget *window, GtkStateType flags, gpointer udata);
static gboolean uiWindowMappedHandler (GtkWidget *window, GdkEventAny *event, gpointer udata);

void
uiCreateMainWindow (UIWidget *uiwidget, callback_t *uicb,
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

void
uiWindowSetTitle (UIWidget *uiwidget, const char *title)
{
  if (title != NULL && uiwidget != NULL && uiwidget->widget != NULL) {
    gtk_window_set_title (GTK_WINDOW (uiwidget->widget), title);
  }
}

void
uiCloseWindow (UIWidget *uiwindow)
{
  if (GTK_IS_WIDGET (uiwindow->widget)) {
    gtk_widget_destroy (uiwindow->widget);
  }
}

bool
uiWindowIsMaximized (UIWidget *uiwindow)
{
  return (bool) gtk_window_is_maximized (GTK_WINDOW (uiwindow->widget));
}

void
uiWindowIconify (UIWidget *uiwindow)
{
  gtk_window_iconify (GTK_WINDOW (uiwindow->widget));
}

void
uiWindowDeIconify (UIWidget *uiwindow)
{
  gtk_window_deiconify (GTK_WINDOW (uiwindow->widget));
}

void
uiWindowMaximize (UIWidget *uiwindow)
{
  gtk_window_maximize (GTK_WINDOW (uiwindow->widget));
}

void
uiWindowUnMaximize (UIWidget *uiwindow)
{
  gtk_window_unmaximize (GTK_WINDOW (uiwindow->widget));
}

void
uiWindowDisableDecorations (UIWidget *uiwindow)
{
  gtk_window_set_decorated (GTK_WINDOW (uiwindow->widget), FALSE);
}

void
uiWindowEnableDecorations (UIWidget *uiwindow)
{
  /* this does not work on windows, the decorations are not recovered */
  /* after being disabled. */
  gtk_window_set_decorated (GTK_WINDOW (uiwindow->widget), TRUE);
}

void
uiWindowGetSize (UIWidget *uiwindow, int *x, int *y)
{
  gtk_window_get_size (GTK_WINDOW (uiwindow->widget), x, y);
}

void
uiWindowSetDefaultSize (UIWidget *uiwindow, int x, int y)
{
  if (x >= 0 && y >= 0) {
    gtk_window_set_default_size (GTK_WINDOW (uiwindow->widget), x, y);
  }
}

void
uiWindowGetPosition (UIWidget *uiwindow, int *x, int *y, int *ws)
{
  GdkWindow *gdkwin;

  gtk_window_get_position (GTK_WINDOW (uiwindow->widget), x, y);
  gdkwin = gtk_widget_get_window (uiwindow->widget);
  *ws = -1;
  if (gdkwin != NULL) {
/* being lazy here, just check for the header to see if it is x11 */
#if _hdr_gdk_gdkx
    *ws = gdk_x11_window_get_desktop (gdkwin);
#endif
  }
}

void
uiWindowMove (UIWidget *uiwindow, int x, int y, int ws)
{
  GdkWindow *gdkwin;

  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (uiwindow->widget), x, y);
  }
  if (ws != -1) {
    gdkwin = gtk_widget_get_window (uiwindow->widget);
    if (gdkwin != NULL) {
#if _hdr_gdk_gdkx
      gdk_x11_window_move_to_desktop (gdkwin, ws);
#endif
    }
  }
}

void
uiWindowMoveToCurrentWorkspace (UIWidget *uiwindow)
{
  GdkWindow *gdkwin;

  gdkwin = gtk_widget_get_window (uiwindow->widget);

  if (gdkwin != NULL) {
/* being lazy here, just check for the header to see if it is x11 */
#if _hdr_gdk_gdkx
    gdk_x11_window_move_to_current_desktop (gdkwin);
#endif
  }
}

void
uiWindowNoFocusOnStartup (UIWidget *uiwindow)
{
  gtk_window_set_focus_on_map (GTK_WINDOW (uiwindow->widget), FALSE);
}

void
uiCreateScrolledWindow (UIWidget *uiwidget, int minheight)
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

  uiwidget->widget = widget;
}

void
uiWindowSetPolicyExternal (UIWidget *uisw)
{
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (uisw->widget),
      GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);
}

void
uiCreateDialogWindow (UIWidget *uiwidget, UIWidget *parentwin,
    UIWidget *attachment, callback_t *uicb, const char *title)
{
  GtkWidget *window;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (window != NULL);
  if (attachment != NULL) {
    gtk_window_set_attached_to (GTK_WINDOW (window), attachment->widget);
  }
  if (parentwin != NULL) {
    gtk_window_set_transient_for (GTK_WINDOW (window),
        GTK_WINDOW (parentwin->widget));
  }
  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
  gtk_window_set_deletable (GTK_WINDOW (window), FALSE);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (window), 6);
  gtk_widget_hide (window);
  gtk_widget_set_vexpand (window, FALSE);
  gtk_widget_set_events (window, GDK_FOCUS_CHANGE_MASK);

  if (title != NULL) {
    gtk_window_set_title (GTK_WINDOW (window), title);
  }
  if (uicb != NULL) {
    g_signal_connect (G_OBJECT (window),
        "focus-out-event", G_CALLBACK (uiWindowFocusOutCallback), uicb);
  }

  uiwidget->widget = window;
}

void
uiWindowSetDoubleClickCallback (UIWidget *uiwidget, callback_t *uicb)
{
  g_signal_connect (uiwidget->widget, "button-press-event",
      G_CALLBACK (uiWindowDoubleClickHandler), uicb);
}

void
uiWindowSetWinStateCallback (UIWidget *uiwindow, callback_t *uicb)
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
uiWindowSetMappedCallback (UIWidget *uiwidget, callback_t *uicb)
{
  g_signal_connect (uiwidget->widget, "map-event",
      G_CALLBACK (uiWindowMappedHandler), uicb);
}

void
uiWindowPresent (UIWidget *uiwindow)
{
  gtk_window_present (GTK_WINDOW (uiwindow->widget));
}

void
uiWindowRaise (UIWidget *uiwindow)
{
  int   x, y, ws;

  uiWindowGetPosition (uiwindow, &x, &y, &ws);
  uiWidgetHide (uiwindow);
  uiWidgetShow (uiwindow);
  uiWindowMove (uiwindow, x, y, ws);
}

void
uiWindowFind (UIWidget *window)
{
  uiWindowMoveToCurrentWorkspace (window);
  uiWindowRaise (window);
}

/* internal routines */

static gboolean
uiWindowFocusOutCallback (GtkWidget *w, GdkEventFocus *event, gpointer udata)
{
  callback_t  *uicb = udata;
  bool        rc = UICB_CONT;

  rc = callbackHandler (uicb);
  return rc;
}

static gboolean
uiWindowCloseCallback (GtkWidget *window, GdkEvent *event, gpointer udata)
{
  callback_t  *uicb = udata;
  bool        rc = UICB_CONT;

  rc = callbackHandler (uicb);
  return rc;
}

static gboolean
uiWindowDoubleClickHandler (GtkWidget *window, GdkEventButton *event, gpointer udata)
{
  callback_t  *uicb = udata;
  bool        rc;

  if (gdk_event_get_event_type ((GdkEvent *) event) != GDK_DOUBLE_BUTTON_PRESS) {
    return UICB_CONT;
  }

  rc = callbackHandler (uicb);
  return rc;
}

static gboolean
uiWindowWinStateHandler (GtkWidget *window, GdkEventWindowState *event, gpointer udata)
{
  callback_t  *uicb = udata;
  bool        rc;
  int         isicon = -1;
  int         ismax = -1;
  bool        process = false;

  if (! callbackIsSet (uicb)) {
    return UICB_CONT;
  }

  if ((event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) == GDK_WINDOW_STATE_ICONIFIED) {
    isicon = false;
    if ((event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) ==
        GDK_WINDOW_STATE_ICONIFIED) {
      isicon = true;
    }
    process = true;
  }
  if ((event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) == GDK_WINDOW_STATE_MAXIMIZED) {
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

  rc = callbackHandlerIntInt (uicb, isicon, ismax);
  return rc;
}

static gboolean
uiWindowMappedHandler (GtkWidget *window, GdkEventAny *event, gpointer udata)
{
  callback_t  *uicb = udata;
  bool        rc = false;

  rc = callbackHandler (uicb);
  return rc;
}


static void
uiWindowNoDimHandler (GtkWidget *window, GtkStateType flags, gpointer udata)
{
  /* never in a backdrop state */
  gtk_widget_unset_state_flags (GTK_WIDGET (window), GTK_STATE_FLAG_BACKDROP);
}

