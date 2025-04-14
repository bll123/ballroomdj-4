/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
  callback_t      *changecb;
  GtkAdjustment   *adjustment;
} uiscrollbar_t;

static gboolean uiScrollbarChangeHandler (GtkRange *range, GtkScrollType scrolltype, gdouble value, gpointer udata);

uiwcont_t *
uiCreateVerticalScrollbar (double upper)
{
  uiwcont_t     *uiwidget;
  uiscrollbar_t *sb;
  GtkWidget     *widget;
  GtkAdjustment *adjustment;

  sb = mdmalloc (sizeof (uiscrollbar_t));
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
  uiscrollbar_t   *sb;

  if (! uiwcontValid (uiwidget, WCONT_T_SCROLLBAR, "sb-set-chg-cb")) {
    return;
  }

  sb = uiwidget->uiint.uiscrollbar;
  sb->changecb = cb;
  uiwidget->uidata.sigid [SIGID_VAL_CHG] =
      g_signal_connect (uiwidget->uidata.widget, "change-value",
      G_CALLBACK (uiScrollbarChangeHandler), cb);
}

void
uiScrollbarSetUpper (uiwcont_t *uiwidget, double upper)
{
  uiscrollbar_t   *sb;

  if (! uiwcontValid (uiwidget, WCONT_T_SCROLLBAR, "sb-set-upper")) {
    return;
  }

  sb = uiwidget->uiint.uiscrollbar;
  gtk_adjustment_set_upper (sb->adjustment, upper);
}

void
uiScrollbarSetPosition (uiwcont_t *uiwidget, double pos)
{
  uiscrollbar_t   *sb;

  if (! uiwcontValid (uiwidget, WCONT_T_SCROLLBAR, "sb-set-pos")) {
    return;
  }

  sb = uiwidget->uiint.uiscrollbar;
  gtk_adjustment_set_value (sb->adjustment, pos);
}

void
uiScrollbarSetStepIncrement (uiwcont_t *uiwidget, double step)
{
  uiscrollbar_t   *sb;

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
  uiscrollbar_t   *sb;

  if (! uiwcontValid (uiwidget, WCONT_T_SCROLLBAR, "sb-set-page-inc")) {
    return;
  }

  sb = uiwidget->uiint.uiscrollbar;
  gtk_adjustment_set_page_increment (sb->adjustment, page);
}

void
uiScrollbarSetPageSize (uiwcont_t *uiwidget, double sz)
{
  uiscrollbar_t   *sb;

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
