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

#include "ui/uiui.h"
#include "ui/uiwidget.h"

/* widget interface */

void
uiWidgetDisable (UIWidget *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }
  gtk_widget_set_sensitive (uiwidget->widget, FALSE);
}

void
uiWidgetEnable (UIWidget *uiwidget)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }
  gtk_widget_set_sensitive (uiwidget->widget, TRUE);
}

void
uiWidgetExpandHoriz (UIWidget *uiwidget)
{
  gtk_widget_set_hexpand (uiwidget->widget, TRUE);
}

void
uiWidgetExpandVert (UIWidget *uiwidget)
{
  gtk_widget_set_vexpand (uiwidget->widget, TRUE);
}

void
uiWidgetSetAllMargins (UIWidget *uiwidget, int mult)
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
uiWidgetSetMarginTop (UIWidget *uiwidget, int mult)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_top (uiwidget->widget, uiBaseMarginSz * mult);
}

void
uiWidgetSetMarginBottom (UIWidget *uiwidget, int mult)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_bottom (uiwidget->widget, uiBaseMarginSz * mult);
}

void
uiWidgetSetMarginStart (UIWidget *uiwidget, int mult)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_start (uiwidget->widget, uiBaseMarginSz * mult);
}


void
uiWidgetSetMarginEnd (UIWidget *uiwidget, int mult)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_margin_end (uiwidget->widget, uiBaseMarginSz * mult);
}

void
uiWidgetAlignHorizFill (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_FILL);
}

void
uiWidgetAlignHorizStart (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_START);
}

void
uiWidgetAlignHorizEnd (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_END);
}

void
uiWidgetAlignHorizCenter (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_halign (uiwidget->widget, GTK_ALIGN_CENTER);
}

void
uiWidgetAlignVertFill (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->widget, GTK_ALIGN_FILL);
}

void
uiWidgetAlignVertStart (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->widget, GTK_ALIGN_START);
}

void
uiWidgetAlignVertCenter (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_valign (uiwidget->widget, GTK_ALIGN_CENTER);
}

void
uiWidgetDisableFocus (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_set_can_focus (uiwidget->widget, FALSE);
}

void
uiWidgetHide (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_hide (uiwidget->widget);
}

void
uiWidgetShow (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_show (uiwidget->widget);
}

void
uiWidgetShowAll (UIWidget *uiwidget)
{
  if (uiwidget->widget == NULL) {
    return;
  }

  gtk_widget_show_all (uiwidget->widget);
}


void
uiWidgetMakePersistent (UIWidget *uiwidget)
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
uiWidgetClearPersistent (UIWidget *uiwidget)
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
uiWidgetSetSizeRequest (UIWidget *uiwidget, int width, int height)
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
uiWidgetIsValid (UIWidget *uiwidget)
{
  bool    rc = false;
  if (uiwidget->widget != NULL) {
    rc = true;
  }
  return rc;
}

void
uiWidgetGetPosition (UIWidget *uiwidget, int *x, int *y)
{
  GtkAllocation alloc;

  gtk_widget_get_allocation (uiwidget->widget, &alloc);
  *x = alloc.x;
  *y = alloc.y;
}

void
uiWidgetSetClass (UIWidget *uiwidget, const char *class)
{
  gtk_style_context_add_class (
     gtk_widget_get_style_context (uiwidget->widget), class);
}

void
uiWidgetRemoveClass (UIWidget *uiwidget, const char *class)
{
  gtk_style_context_remove_class (
      gtk_widget_get_style_context (uiwidget->widget), class);
}

