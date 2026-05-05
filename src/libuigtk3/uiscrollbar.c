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
#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiwidget.h"
#include "ui/uiscrollbar.h"

typedef struct uiscrollbarint {
  callback_t      *changecb;
  GtkAdjustment   *adjustment;
} uiscrollbarint_t;

static gboolean uiScrollbarChangeHandler (GtkRange *range, GtkScrollType scrolltype, gdouble value, gpointer udata);

uiwcont_t *
uiCreateVerticalScrollbar (double upper)
{
  uiwcont_t     *uiwidget;
  uiscrollbarint_t *sb;
  GtkWidget     *widget;
  GtkAdjustment *adjustment;

  sb = mdmalloc (sizeof (uiscrollbarint_t));
  sb->changecb = NULL;

  adjustment = gtk_adjustment_new (0.0, 0.0, upper, 1.0, 10.0, 10.0);
  widget = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adjustment);
  sb->adjustment = adjustment;

  uiwidget = uiwcontAlloc (WCONT_T_SCROLLBAR, WCONT_T_SCROLLBAR);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uiscrollbar = sb;

  uiWidgetExpandVert (uiwidget);

  return uiwidget;
}

/* only frees up the internals */
void
uiScrollbarFree (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SCROLLBAR, "sb-free")) {
    return;
  }

  dataFree (uiwidget->uiint.uiscrollbar);
  /* the container is freed by uiwcontFree() */
}

void
uiScrollbarSetChangeCallback (uiwcont_t *uiwidget, callback_t *cb)
{
  uiscrollbarint_t   *sb;

  if (! uiwcontValid (uiwidget, WCONT_T_SCROLLBAR, "sb-set-chg-cb")) {
    return;
  }

  sb = uiwidget->uiint.uiscrollbar;
  sb->changecb = cb;
  g_signal_connect (uiwidget->widget, "change-value",
      G_CALLBACK (uiScrollbarChangeHandler), cb);
}

void
uiScrollbarSetUpper (uiwcont_t *uiwidget, double upper)
{
  uiscrollbarint_t   *sb;

  if (! uiwcontValid (uiwidget, WCONT_T_SCROLLBAR, "sb-set-upper")) {
    return;
  }

  sb = uiwidget->uiint.uiscrollbar;
  gtk_adjustment_set_upper (sb->adjustment, upper);
}

void
uiScrollbarSetPosition (uiwcont_t *uiwidget, double pos)
{
  uiscrollbarint_t   *sb;

  if (! uiwcontValid (uiwidget, WCONT_T_SCROLLBAR, "sb-set-pos")) {
    return;
  }

  sb = uiwidget->uiint.uiscrollbar;
  gtk_adjustment_set_value (sb->adjustment, pos);
}

void
uiScrollbarSetStepIncrement (uiwcont_t *uiwidget, double step)
{
  uiscrollbarint_t   *sb;

  if (! uiwcontValid (uiwidget, WCONT_T_SCROLLBAR, "sb-set-step-inc")) {
    return;
  }

  if (uiwidget == NULL || uiwidget->wtype != WCONT_T_SCROLLBAR) {
    return;
  }

  sb = uiwidget->uiint.uiscrollbar;
  gtk_adjustment_set_step_increment (sb->adjustment, step);
}

void
uiScrollbarSetPageIncrement (uiwcont_t *uiwidget, double page)
{
  uiscrollbarint_t   *sb;

  if (! uiwcontValid (uiwidget, WCONT_T_SCROLLBAR, "sb-set-page-inc")) {
    return;
  }

  sb = uiwidget->uiint.uiscrollbar;
  gtk_adjustment_set_page_increment (sb->adjustment, page);
}

void
uiScrollbarSetPageSize (uiwcont_t *uiwidget, double sz)
{
  uiscrollbarint_t   *sb;

  if (! uiwcontValid (uiwidget, WCONT_T_SCROLLBAR, "sb-set-page-sz")) {
    return;
  }

  sb = uiwidget->uiint.uiscrollbar;
  gtk_adjustment_set_page_size (sb->adjustment, sz);
}

/* internal routines */

static gboolean
uiScrollbarChangeHandler (GtkRange *range, GtkScrollType scrolltype,
    gdouble value, gpointer udata)
{
  callback_t    *cb = udata;
  bool          rc = false;

  if (cb != NULL) {
    rc = callbackHandlerD (cb, value);
  }

  return rc;
}
