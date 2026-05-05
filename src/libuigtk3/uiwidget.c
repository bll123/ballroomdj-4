/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"

static gboolean uiWidgetSizeChgHandler (GtkWidget *w, GtkAllocation *allocation, gpointer udata);
static gboolean uiWidgetEnterHandler (GtkWidget *w, GdkEventCrossing *event, gpointer udata);

/* widget interface */

void
uiWidgetSetState (uiwcont_t *uiwidget, int state)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  if (GTK_IS_WIDGET (uiwidget->widget)) {
    gtk_widget_set_sensitive (uiwidget->widget, state);
  }
}

/* this uses the GTK terminology */
/* set the widget to take any horizontal space */
void
uiWidgetExpandHoriz (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  if (GTK_IS_WIDGET (uiwidget->widget)) {
    gtk_widget_set_hexpand (uiwidget->widget, TRUE);
  }
}

/* this uses the GTK terminology */
/* set the widget to take any horizontal space */
void
uiWidgetExpandVert (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_vexpand (uiwidget->widget, TRUE);
}

void
uiWidgetSetAllMargins (uiwcont_t *uiwidget, int mult)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_top (uiwidget->widget, uiBaseMarginSz * mult);
  gtk_widget_set_margin_bottom (uiwidget->widget, uiBaseMarginSz * mult);
  gtk_widget_set_margin_start (uiwidget->widget, uiBaseMarginSz * mult);
  gtk_widget_set_margin_end (uiwidget->widget, uiBaseMarginSz * mult);
}

void
uiWidgetSetMarginTop (uiwcont_t *uiwidget, int mult)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_top (uiwidget->widget, uiBaseMarginSz * mult);
}

void
uiWidgetSetMarginBottom (uiwcont_t *uiwidget, int mult)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_bottom (uiwidget->widget, uiBaseMarginSz * mult);
}

void
uiWidgetSetMarginStart (uiwcont_t *uiwidget, int mult)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_start (uiwidget->widget, uiBaseMarginSz * mult);
}


void
uiWidgetSetMarginEnd (uiwcont_t *uiwidget, int mult)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_end (uiwidget->widget, uiBaseMarginSz * mult);
}

/* this uses the GTK terminology */
/* controls how the widget handles extra space allocated to it */
void
uiWidgetAlignHorizFill (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_FILL);
}

void
uiWidgetAlignHorizStart (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_START);
}

void
uiWidgetAlignHorizEnd (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_END);
}

void
uiWidgetAlignHorizCenter (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_CENTER);
}

void
uiWidgetAlignVertFill (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->widget, GTK_ALIGN_FILL);
}

void
uiWidgetAlignVertStart (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->widget, GTK_ALIGN_START);
}

void
uiWidgetAlignVertCenter (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->widget, GTK_ALIGN_CENTER);
}

void
uiWidgetAlignVertEnd (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->widget, GTK_ALIGN_END);
}

void
uiWidgetAlignVertBaseline (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->widget, GTK_ALIGN_BASELINE);
}

void
uiWidgetDisableFocus (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_can_focus (uiwidget->widget, FALSE);
}

void
uiWidgetEnableFocus (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_can_focus (uiwidget->widget, TRUE);
  gtk_widget_set_focus_on_click (uiwidget->widget, TRUE);
}

void
uiWidgetGrabFocus (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_grab_focus (uiwidget->widget);
}

void
uiWidgetHide (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_hide (uiwidget->widget);
}

void
uiWidgetShow (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_show (uiwidget->widget);
}

void
uiWidgetShowAll (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_show_all (uiwidget->widget);
}

void
uiWidgetSetSizeRequest (uiwcont_t *uiwidget, int width, int height)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_size_request (uiwidget->widget, width, height);
}

bool
uiWidgetIsMapped (uiwcont_t *uiwidget)
{
  bool    rc = false;

  if (uiwidget == NULL) {
    return rc;
  }
  if (uiwidget->widget == NULL) {
    return rc;
  }

  rc = gtk_widget_get_mapped (uiwidget->widget);
  return rc;
}


void
uiWidgetGetPosition (uiwcont_t *uiwidget, int *x, int *y)
{
  GtkAllocation alloc;

  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_get_allocation (uiwidget->widget, &alloc);
  *x = alloc.x;
  *y = alloc.y;
}


void
uiWidgetSetClass (uiwcont_t *uiwidget, const char *class)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }
  if (class == NULL) {
    return;
  }

  gtk_style_context_add_class (
     gtk_widget_get_style_context (uiwidget->widget), class);
}

void
uiWidgetClearClass (uiwcont_t *uiwidget, const char *class)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_style_context_remove_class (
      gtk_widget_get_style_context (uiwidget->widget), class);
}

void
uiWidgetSetTooltip (uiwcont_t *uiwidget, const char *tooltip)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_tooltip_text (uiwidget->widget, tooltip);
}

void
uiWidgetSetSizeChgCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }

  g_signal_connect (uiwidget->widget, "size-allocate",
      G_CALLBACK (uiWidgetSizeChgHandler), uicb);
}

void
uiWidgetSetEnterCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_add_events (uiwidget->widget, GDK_ENTER_NOTIFY_MASK);
  g_signal_connect (uiwidget->widget, "enter-notify-event",
      G_CALLBACK (uiWidgetEnterHandler), uicb);
}

/* internal routines */

static gboolean
uiWidgetSizeChgHandler (GtkWidget *w, GtkAllocation *allocation, gpointer udata)
{
  callback_t  *uicb = udata;
  bool        rc = false;

  if (uicb != NULL) {
    rc = callbackHandlerII (uicb, allocation->width, allocation->height);
  }
  return rc;
}

static gboolean
uiWidgetEnterHandler (GtkWidget *w, GdkEventCrossing *event, gpointer udata)
{
  callback_t  *uicb = udata;
  bool        rc = false;

  if (uicb != NULL) {
    rc = callbackHandler (uicb);
  }
  return rc;
}
