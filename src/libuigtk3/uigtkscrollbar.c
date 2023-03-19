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
#include "ui/uiwidget.h"
#include "ui/uiscrollbar.h"

void
uiCreateVerticalScrollbar (uiwidget_t *uiwidget, double upper)
{
  GtkWidget     *sb;
  GtkAdjustment *adjustment;

  adjustment = gtk_adjustment_new (0.0, 0.0, upper, 1.0, 10.0, 10.0);
  sb = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adjustment);
  uiwidget->widget = sb;
  uiWidgetExpandVert (uiwidget);
}

void
uiScrollbarSetUpper (uiwidget_t *uisb, double upper)
{
  GtkAdjustment   *adjustment;

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uisb->widget));
  gtk_adjustment_set_upper (adjustment, upper);
}

void
uiScrollbarSetPosition (uiwidget_t *uisb, double pos)
{
  GtkAdjustment   *adjustment;

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uisb->widget));
  gtk_adjustment_set_value (adjustment, pos);
}

void
uiScrollbarSetStepIncrement (uiwidget_t *uisb, double step)
{
  GtkAdjustment   *adjustment;

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uisb->widget));
  gtk_adjustment_set_step_increment (adjustment, step);
}

void
uiScrollbarSetPageIncrement (uiwidget_t *uisb, double page)
{
  GtkAdjustment   *adjustment;

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uisb->widget));
  gtk_adjustment_set_page_increment (adjustment, page);
}

void
uiScrollbarSetPageSize (uiwidget_t *uisb, double sz)
{
  GtkAdjustment   *adjustment;

  adjustment = gtk_range_get_adjustment (GTK_RANGE (uisb->widget));
  gtk_adjustment_set_page_size (adjustment, sz);
}

