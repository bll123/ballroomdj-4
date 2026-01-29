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
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  if (GTK_IS_WIDGET (uiwidget->uidata.widget)) {
    gtk_widget_set_sensitive (uiwidget->uidata.widget, state);
  }
}

/* this uses the GTK terminology */
/* set the widget to take any horizontal space */
void
uiWidgetExpandHoriz (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  if (GTK_IS_WIDGET (uiwidget->uidata.widget)) {
    gtk_widget_set_hexpand (uiwidget->uidata.widget, TRUE);
  }
}

/* this uses the GTK terminology */
/* set the widget to take any horizontal space */
void
uiWidgetExpandVert (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_vexpand (uiwidget->uidata.widget, TRUE);
}

void
uiWidgetSetAllMargins (uiwcont_t *uiwidget, int mult)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_margin_top (uiwidget->uidata.widget, uiBaseMarginSz * mult);
  gtk_widget_set_margin_bottom (uiwidget->uidata.widget, uiBaseMarginSz * mult);
  gtk_widget_set_margin_start (uiwidget->uidata.widget, uiBaseMarginSz * mult);
  gtk_widget_set_margin_end (uiwidget->uidata.widget, uiBaseMarginSz * mult);
}

void
uiWidgetSetMarginTop (uiwcont_t *uiwidget, int mult)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_margin_top (uiwidget->uidata.widget, uiBaseMarginSz * mult);
}

void
uiWidgetSetMarginBottom (uiwcont_t *uiwidget, int mult)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_margin_bottom (uiwidget->uidata.widget, uiBaseMarginSz * mult);
}

void
uiWidgetSetMarginStart (uiwcont_t *uiwidget, int mult)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_margin_start (uiwidget->uidata.widget, uiBaseMarginSz * mult);
}


void
uiWidgetSetMarginEnd (uiwcont_t *uiwidget, int mult)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_margin_end (uiwidget->uidata.widget, uiBaseMarginSz * mult);
}

/* this uses the GTK terminology */
/* controls how the widget handles extra space allocated to it */
void
uiWidgetAlignHorizFill (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->uidata.widget, GTK_ALIGN_FILL);
}

void
uiWidgetAlignHorizStart (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->uidata.widget, GTK_ALIGN_START);
}

void
uiWidgetAlignHorizEnd (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->uidata.widget, GTK_ALIGN_END);
}

void
uiWidgetAlignHorizCenter (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->uidata.widget, GTK_ALIGN_CENTER);
}

void
uiWidgetAlignVertFill (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->uidata.widget, GTK_ALIGN_FILL);
}

void
uiWidgetAlignVertStart (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->uidata.widget, GTK_ALIGN_START);
}

void
uiWidgetAlignVertCenter (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->uidata.widget, GTK_ALIGN_CENTER);
}

void
uiWidgetAlignVertEnd (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->uidata.widget, GTK_ALIGN_END);
}

void
uiWidgetAlignVertBaseline (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->uidata.widget, GTK_ALIGN_BASELINE);
}

void
uiWidgetDisableFocus (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_can_focus (uiwidget->uidata.widget, FALSE);
}

void
uiWidgetEnableFocus (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_can_focus (uiwidget->uidata.widget, TRUE);
  gtk_widget_set_focus_on_click (uiwidget->uidata.widget, TRUE);
}

void
uiWidgetGrabFocus (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_grab_focus (uiwidget->uidata.widget);
}

void
uiWidgetHide (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_hide (uiwidget->uidata.widget);
}

void
uiWidgetShow (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_show (uiwidget->uidata.widget);
}

void
uiWidgetShowAll (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_show_all (uiwidget->uidata.widget);
}


void
uiWidgetMakePersistent (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->uidata.widget == NULL) {
    return;
  }

  if (G_IS_OBJECT (uiwidget->uidata.widget)) {
    g_object_ref_sink (G_OBJECT (uiwidget->uidata.widget));
  }
}

void
uiWidgetClearPersistent (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->uidata.widget == NULL) {
    return;
  }

  if (G_IS_OBJECT (uiwidget->uidata.widget)) {
    g_object_unref (G_OBJECT (uiwidget->uidata.widget));
  }
}

void
uiWidgetSetSizeRequest (uiwcont_t *uiwidget, int width, int height)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_size_request (uiwidget->uidata.widget, width, height);
}

bool
uiWidgetIsMapped (uiwcont_t *uiwidget)
{
  bool    rc = false;

  if (uiwidget == NULL) {
    return rc;
  }
  if (uiwidget->uidata.widget == NULL) {
    return rc;
  }

  rc = gtk_widget_get_mapped (uiwidget->uidata.widget);
  return rc;
}


void
uiWidgetGetPosition (uiwcont_t *uiwidget, int *x, int *y)
{
  GtkAllocation alloc;

  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_get_allocation (uiwidget->uidata.widget, &alloc);
  *x = alloc.x;
  *y = alloc.y;
}


void
uiWidgetAddClass (uiwcont_t *uiwidget, const char *class)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_style_context_add_class (
     gtk_widget_get_style_context (uiwidget->uidata.widget), class);
}

void
uiWidgetRemoveClass (uiwcont_t *uiwidget, const char *class)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_style_context_remove_class (
      gtk_widget_get_style_context (uiwidget->uidata.widget), class);
}

void
uiWidgetSetTooltip (uiwcont_t *uiwidget, const char *tooltip)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_set_tooltip_text (uiwidget->uidata.widget, tooltip);
}

void
uiWidgetSetSizeChgCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->uidata.widget == NULL) {
    return;
  }

  g_signal_connect (uiwidget->uidata.widget, "size-allocate",
      G_CALLBACK (uiWidgetSizeChgHandler), uicb);
}

void
uiWidgetSetEnterCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->uidata.widget == NULL) {
    return;
  }

  gtk_widget_add_events (uiwidget->uidata.widget, GDK_ENTER_NOTIFY_MASK);
  g_signal_connect (uiwidget->uidata.widget, "enter-notify-event",
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
