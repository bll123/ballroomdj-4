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

#include <gtk/gtk.h>

#include "ui/uiinternal.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

/* widget interface */

void
uiWidgetSetState (uiwidget_t *uiwidget, int state)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }
  gtk_widget_set_sensitive (uiwidget->widget, state);
}

void
uiWidgetExpandHoriz (uiwidget_t *uiwidget)
{
  gtk_widget_set_hexpand (uiwidget->widget, TRUE);
}

void
uiWidgetExpandVert (uiwidget_t *uiwidget)
{
  gtk_widget_set_vexpand (uiwidget->widget, TRUE);
}

void
uiWidgetSetAllMargins (uiwidget_t *uiwidget, int mult)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_top (uiwidget->widget, uiBaseMarginSz * mult);
  gtk_widget_set_margin_bottom (uiwidget->widget, uiBaseMarginSz * mult);
  gtk_widget_set_margin_start (uiwidget->widget, uiBaseMarginSz * mult);
  gtk_widget_set_margin_end (uiwidget->widget, uiBaseMarginSz * mult);
}

void
uiWidgetSetMarginTop (uiwidget_t *uiwidget, int mult)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_top (uiwidget->widget, uiBaseMarginSz * mult);
}

void
uiWidgetSetMarginBottom (uiwidget_t *uiwidget, int mult)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_bottom (uiwidget->widget, uiBaseMarginSz * mult);
}

void
uiWidgetSetMarginStart (uiwidget_t *uiwidget, int mult)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_start (uiwidget->widget, uiBaseMarginSz * mult);
}


void
uiWidgetSetMarginEnd (uiwidget_t *uiwidget, int mult)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_end (uiwidget->widget, uiBaseMarginSz * mult);
}

void
uiWidgetAlignHorizFill (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_FILL);
}

void
uiWidgetAlignHorizStart (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_START);
}

void
uiWidgetAlignHorizEnd (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_END);
}

void
uiWidgetAlignHorizCenter (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_CENTER);
}

void
uiWidgetAlignVertFill (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->widget, GTK_ALIGN_FILL);
}

void
uiWidgetAlignVertStart (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->widget, GTK_ALIGN_START);
}

void
uiWidgetAlignVertCenter (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->widget, GTK_ALIGN_CENTER);
}

void
uiWidgetDisableFocus (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_can_focus (uiwidget->widget, FALSE);
}

void
uiWidgetHide (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_hide (uiwidget->widget);
}

void
uiWidgetShow (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_show (uiwidget->widget);
}

void
uiWidgetShowAll (uiwidget_t *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_show_all (uiwidget->widget);
}


void
uiWidgetMakePersistent (uiwidget_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }

  if (G_IS_OBJECT (uiwidget->widget)) {
    g_object_ref_sink (G_OBJECT (uiwidget->widget));
  }
}

void
uiWidgetClearPersistent (uiwidget_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->widget == NULL) {
    return;
  }

  if (G_IS_OBJECT (uiwidget->widget)) {
    g_object_unref (G_OBJECT (uiwidget->widget));
  }
}

void
uiWidgetSetSizeRequest (uiwidget_t *uiwidget, int width, int height)
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
uiWidgetIsValid (uiwidget_t *uiwidget)
{
  bool    rc = false;
  if (uiwidget->widget != NULL) {
    rc = true;
  }
  return rc;
}

void
uiWidgetGetPosition (uiwidget_t *uiwidget, int *x, int *y)
{
  GtkAllocation alloc;

  gtk_widget_get_allocation (uiwidget->widget, &alloc);
  *x = alloc.x;
  *y = alloc.y;
}

void
uiWidgetSetClass (uiwidget_t *uiwidget, const char *class)
{
  gtk_style_context_add_class (
     gtk_widget_get_style_context (uiwidget->widget), class);
}

void
uiWidgetRemoveClass (uiwidget_t *uiwidget, const char *class)
{
  gtk_style_context_remove_class (
      gtk_widget_get_style_context (uiwidget->widget), class);
}

