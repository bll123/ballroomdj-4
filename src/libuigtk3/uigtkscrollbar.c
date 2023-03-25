/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "callback.h"
#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiwidget.h"
#include "ui/uiscrollbar.h"

typedef struct uiscrollbar {
  uiwcont_t     *wcont;
  callback_t    *changecb;
} uiscrollbar_t;

static gboolean uiScrollbarChangeHandler (GtkRange *range, GtkScrollType scrolltype, gdouble value, gpointer udata);

uiscrollbar_t *
uiCreateVerticalScrollbar (double upper)
{
  uiscrollbar_t *sb;
  GtkWidget     *widget;
  GtkAdjustment *adjustment;

  sb = mdmalloc (sizeof (uiscrollbar_t));
  sb->changecb = NULL;
  sb->wcont = uiwcontAlloc ();

  adjustment = gtk_adjustment_new (0.0, 0.0, upper, 1.0, 10.0, 10.0);
  widget = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adjustment);
  sb->wcont->widget = widget;

  uiWidgetExpandVert (sb->wcont);
  return sb;
}

void
uiScrollbarFree (uiscrollbar_t *sb)
{
  if (sb == NULL) {
    return;
  }

  uiwcontFree (sb->wcont);
  mdfree (sb);
}

uiwcont_t *
uiScrollbarGetWidgetContainer (uiscrollbar_t *sb)
{
  if (sb == NULL) {
    return NULL;
  }

  return sb->wcont;
}

void
uiScrollbarSetChangeCallback (uiscrollbar_t *sb, callback_t *cb)
{
  if (sb == NULL) {
    return;
  }

  sb->changecb = cb;
  g_signal_connect (sb->wcont->widget, "change-value",
      G_CALLBACK (uiScrollbarChangeHandler), cb);
}

void
uiScrollbarSetUpper (uiscrollbar_t *sb, double upper)
{
  GtkAdjustment   *adjustment;

  adjustment = gtk_range_get_adjustment (GTK_RANGE (sb->wcont->widget));
  gtk_adjustment_set_upper (adjustment, upper);
}

void
uiScrollbarSetPosition (uiscrollbar_t *sb, double pos)
{
  GtkAdjustment   *adjustment;

  adjustment = gtk_range_get_adjustment (GTK_RANGE (sb->wcont->widget));
  gtk_adjustment_set_value (adjustment, pos);
}

void
uiScrollbarSetStepIncrement (uiscrollbar_t *sb, double step)
{
  GtkAdjustment   *adjustment;

  adjustment = gtk_range_get_adjustment (GTK_RANGE (sb->wcont->widget));
  gtk_adjustment_set_step_increment (adjustment, step);
}

void
uiScrollbarSetPageIncrement (uiscrollbar_t *sb, double page)
{
  GtkAdjustment   *adjustment;

  adjustment = gtk_range_get_adjustment (GTK_RANGE (sb->wcont->widget));
  gtk_adjustment_set_page_increment (adjustment, page);
}

void
uiScrollbarSetPageSize (uiscrollbar_t *sb, double sz)
{
  GtkAdjustment   *adjustment;

  adjustment = gtk_range_get_adjustment (GTK_RANGE (sb->wcont->widget));
  gtk_adjustment_set_page_size (adjustment, sz);
}

/* internal routines */

static gboolean
uiScrollbarChangeHandler (GtkRange *range, GtkScrollType scrolltype,
    gdouble value, gpointer udata)
{
  callback_t    *cb = udata;
  bool          rc = false;

  if (cb != NULL) {
    rc = callbackHandlerDouble (cb, value);
  }

  return rc;
}
