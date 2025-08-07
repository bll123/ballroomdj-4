/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>
#if __has_include (<gdk/gdkx.h>)
# include <gdk/gdkx.h>
#endif

#include "callback.h"
#include "uiwcont.h"

#include "ui-gtk3.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

static gboolean uiWindowFocusOutCallback (GtkWidget *w, GdkEventFocus *event, gpointer udata);
static gboolean uiWindowCloseCallback (GtkWidget *window, GdkEvent *event, gpointer udata);
static gboolean uiWindowDoubleClickHandler (GtkWidget *window, GdkEventButton *event, gpointer udata);
static gboolean uiWindowWinStateHandler (GtkWidget *window, GdkEventWindowState *event, gpointer udata);
static void uiWindowNoDimHandler (GtkWidget *window, GtkStateType flags, gpointer udata);

uiwcont_t *
uiCreateMainWindow (callback_t *uicb, const char *title, const char *imagenm)
{
  uiwcont_t     *uiwin;
  GtkWidget     *window;
  GtkIconTheme  *icontheme;

  icontheme = gtk_icon_theme_get_default ();
  gtk_icon_theme_append_search_path (icontheme,
      "/home/bll/s/bdj4/img");

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_widget_set_events (window, GDK_STRUCTURE_MASK);
  gtk_window_set_default_icon_name ("bdj4_icon");
  if (imagenm != NULL) {
    gtk_window_set_icon_name (GTK_WINDOW (window), imagenm);
  }
  if (title != NULL) {
    gtk_window_set_title (GTK_WINDOW (window), title);
  }

  uiwin = uiwcontAlloc (WCONT_T_WINDOW, WCONT_T_WINDOW);
  uiwcontSetWidget (uiwin, window, NULL);

  if (uicb != NULL) {
    g_signal_connect (window, "delete-event",
        G_CALLBACK (uiWindowCloseCallback), uicb);
  }

  return uiwin;
}

void
uiWindowSetTitle (uiwcont_t *uiwindow, const char *title)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-title")) {
    return;
  }
  if (title == NULL) {
    return;
  }

  gtk_window_set_title (GTK_WINDOW (uiwindow->uidata.widget), title);
}

void
uiCloseWindow (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-close")) {
    return;
  }

  if (GTK_IS_WIDGET (uiwindow->uidata.widget)) {
    gtk_widget_destroy (uiwindow->uidata.widget);
  }
}

bool
uiWindowIsMaximized (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-is-max")) {
    return false;
  }

  return (bool) gtk_window_is_maximized (GTK_WINDOW (uiwindow->uidata.widget));
}

void
uiWindowIconify (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-iconify")) {
    return;
  }

  gtk_window_iconify (GTK_WINDOW (uiwindow->uidata.widget));
}

void
uiWindowDeIconify (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-deiconify")) {
    return;
  }

  gtk_window_deiconify (GTK_WINDOW (uiwindow->uidata.widget));
}

void
uiWindowMaximize (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-maximize")) {
    return;
  }

  gtk_window_maximize (GTK_WINDOW (uiwindow->uidata.widget));
}

void
uiWindowUnMaximize (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-unmaximize")) {
    return;
  }

  gtk_window_unmaximize (GTK_WINDOW (uiwindow->uidata.widget));
}

void
uiWindowDisableDecorations (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-disable-dec")) {
    return;
  }

  gtk_window_set_decorated (GTK_WINDOW (uiwindow->uidata.widget), FALSE);
}

void
uiWindowEnableDecorations (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-enable-dec")) {
    return;
  }

  /* this does not work on windows, the decorations are not recovered */
  /* after being disabled. */
  gtk_window_set_decorated (GTK_WINDOW (uiwindow->uidata.widget), TRUE);
}

void
uiWindowGetSize (uiwcont_t *uiwindow, int *x, int *y)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-get-sz")) {
    return;
  }

  gtk_window_get_size (GTK_WINDOW (uiwindow->uidata.widget), x, y);
}

void
uiWindowSetDefaultSize (uiwcont_t *uiwindow, int x, int y)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-dflt-sz")) {
    return;
  }

  if (x >= 0 && y >= 0) {
    gtk_window_set_default_size (GTK_WINDOW (uiwindow->uidata.widget), x, y);
  }
}

void
uiWindowGetPosition (uiwcont_t *uiwindow, int *x, int *y, int *ws)
{
  GdkWindow *gdkwin;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-get-pos")) {
    return;
  }

  gtk_window_get_position (GTK_WINDOW (uiwindow->uidata.widget), x, y);
  gdkwin = gtk_widget_get_window (uiwindow->uidata.widget);
  *ws = -1;
  if (gdkwin != NULL) {
/* being lazy here, just check for the header to see if it is x11 */
#if __has_include (<gdk/gdkx.h>)
    *ws = gdk_x11_window_get_desktop (gdkwin);
#endif
  }
}

void
uiWindowMove (uiwcont_t *uiwindow, int x, int y, int ws)
{
  GdkWindow *gdkwin;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-move")) {
    return;
  }

  if (x >= 0 && y>= 0) {
    gtk_window_move (GTK_WINDOW (uiwindow->uidata.widget), x, y);
  }
  if (ws >= 0) {
    gdkwin = gtk_widget_get_window (uiwindow->uidata.widget);
    if (gdkwin != NULL) {
#if __has_include (<gdk/gdkx.h>)
      gdk_x11_window_move_to_desktop (gdkwin, ws);
#endif
    }
  }
}

void
uiWindowMoveToCurrentWorkspace (uiwcont_t *uiwindow)
{
  GdkWindow *gdkwin;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-move-curr-ws")) {
    return;
  }

  gdkwin = gtk_widget_get_window (uiwindow->uidata.widget);

  if (gdkwin != NULL) {
/* being lazy here, just check for the header to see if it is x11 */
#if __has_include (<gdk/gdkx.h>)
    gdk_x11_window_move_to_current_desktop (gdkwin);
#endif
  }
}

void
uiWindowNoFocusOnStartup (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-no-focus-startup")) {
    return;
  }

  gtk_window_set_focus_on_map (GTK_WINDOW (uiwindow->uidata.widget), FALSE);
}

uiwcont_t *
uiCreateScrolledWindow (int minheight)
{
  uiwcont_t   *scwindow;
  GtkWidget   *widget;

  widget = gtk_scrolled_window_new (NULL, NULL);
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

  scwindow = uiwcontAlloc (WCONT_T_WINDOW, WCONT_T_SCROLL_WINDOW);
  uiwcontSetWidget (scwindow, widget, NULL);
  return scwindow;
}

void
uiWindowSetPolicyExternal (uiwcont_t *uiscw)
{
  if (! uiwcontValid (uiscw, WCONT_T_WINDOW, "win-set-policy-ext")) {
    return;
  }

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (uiscw->uidata.widget),
      GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);
}

uiwcont_t *
uiCreateDialogWindow (uiwcont_t *parentwin,
    uiwcont_t *attachment, callback_t *uicb, const char *title)
{
  uiwcont_t *uiwindow;
  GtkWidget *window;

  if (! uiwcontValid (parentwin, WCONT_T_WINDOW, "win-create-dialog-win")) {
    return NULL;
  }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (attachment != NULL) {
    gtk_window_set_attached_to (GTK_WINDOW (window), attachment->uidata.widget);
  }
  if (parentwin != NULL) {
    gtk_window_set_transient_for (GTK_WINDOW (window),
        GTK_WINDOW (parentwin->uidata.widget));
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

  uiwindow = uiwcontAlloc (WCONT_T_WINDOW, WCONT_T_DIALOG_WINDOW);
  uiwcontSetWidget (uiwindow, window, NULL);
  return uiwindow;
}

void
uiWindowSetDoubleClickCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-dclick-cb")) {
    return;
  }

  g_signal_connect (uiwindow->uidata.widget, "button-press-event",
      G_CALLBACK (uiWindowDoubleClickHandler), uicb);
}

void
uiWindowSetWinStateCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-state-cb")) {
    return;
  }

  g_signal_connect (uiwindow->uidata.widget, "window-state-event",
      G_CALLBACK (uiWindowWinStateHandler), uicb);
}

void
uiWindowNoDim (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-no-dim")) {
    return;
  }

  g_signal_connect (uiwindow->uidata.widget, "state-flags-changed",
      G_CALLBACK (uiWindowNoDimHandler), uiwindow);
}

void
uiWindowPresent (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-present")) {
    return;
  }

  /* this does not work on mac os */
  gtk_window_present (GTK_WINDOW (uiwindow->uidata.widget));
}

void
uiWindowRaise (uiwcont_t *uiwindow)
{
  int   x, y, ws;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-raise")) {
    return;
  }

  uiWindowGetPosition (uiwindow, &x, &y, &ws);
  uiWidgetHide (uiwindow);
  uiWidgetShow (uiwindow);
  uiWindowMove (uiwindow, x, y, ws);
}

void
uiWindowFind (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-find")) {
    return;
  }

  uiWindowMoveToCurrentWorkspace (uiwindow);
  uiWindowRaise (uiwindow);
  uiWindowDeIconify (uiwindow);
}

void
uiWindowPackInWindow (uiwcont_t *uiwindow, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-pack-in-win-win")) {
    return;
  }
  /* the type of the uiwidget is not known */
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_container_add (GTK_CONTAINER (uiwindow->uidata.widget), uiwidget->uidata.widget);
  uiwidget->packed = true;
}

void
uiWindowSetNoMaximize (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_WINDOW, "window-set-no-maximize")) {
    return;
  }

  gtk_window_set_resizable (GTK_WINDOW (uiwidget->uidata.widget), false);
}

void
uiWindowClearFocus (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_WINDOW, "window-clear-focus")) {
    return;
  }

  gtk_window_set_focus (GTK_WINDOW (uiwidget->uidata.widget), NULL);
}

void
uiWindowGetMonitorSize (uiwcont_t *uiwindow, int *width, int *height)
{
  GdkWindow     *gdkwin;
  GdkScreen     *screen;
  GdkDisplay    *display;
  GdkMonitor    *monitor;
  GdkRectangle  rect;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-get-mon-sz")) {
    return;
  }

  gdkwin = gtk_widget_get_window (uiwindow->uidata.widget);
  screen = gtk_window_get_screen (GTK_WINDOW (uiwindow->uidata.widget));
  display = gdk_screen_get_display (screen);
  monitor = gdk_display_get_monitor_at_window (display, gdkwin);
  gdk_monitor_get_geometry (monitor, &rect);
  *width = rect.width - rect.x;
  *height = rect.height - rect.y;
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
  bool        rc = UICB_CONT;
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

  if (uicb != NULL) {
    rc = callbackHandlerII (uicb, isicon, ismax);
  }
  return rc;
}

static void
uiWindowNoDimHandler (GtkWidget *window, GtkStateType flags, gpointer udata)
{
  /* never in a backdrop state */
  gtk_widget_unset_state_flags (GTK_WIDGET (window), GTK_STATE_FLAG_BACKDROP);
}

