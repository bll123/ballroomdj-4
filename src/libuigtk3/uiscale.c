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

#include "callback.h"
#include "uiwcont.h"

#include "ui-gtk3.h"

#include "ui/uiwcont-int.h"

#include "ui/uiwidget.h"
#include "ui/uiscale.h"

static gboolean uiScaleChangeValueHandler (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer udata);

uiwcont_t *
uiCreateScale (double lower, double upper,
    double stepinc, double pageinc, double initvalue, int digits)
{
  uiwcont_t     *uiwidget;
  GtkWidget     *scale;
  GtkAdjustment *adjustment;

  adjustment = gtk_adjustment_new (0.0, lower, upper, stepinc, pageinc, 0.0);
  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);
  gtk_widget_set_size_request (scale, 200, 5);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
  /* the problem with the gtk scale drawing routine is that */
  /* the label is displayed in a disabled color, and there's no easy way */
  /* to align it properly */
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_scale_set_digits (GTK_SCALE (scale), digits);
  gtk_range_set_round_digits (GTK_RANGE (scale), digits);
  gtk_scale_set_has_origin (GTK_SCALE (scale), TRUE);
  gtk_range_set_value (GTK_RANGE (scale), initvalue);

  uiwidget = uiwcontAlloc (WCONT_T_SCALE, WCONT_T_SCALE);
  uiwcontSetWidget (uiwidget, scale, NULL);
//  uiwidget->uidata.widget = scale;
//  uiwidget->uidata.packwidget = scale;

  return uiwidget;
}

void
uiScaleSetCallback (uiwcont_t *uiscale, callback_t *uicb)
{
  if (! uiwcontValid (uiscale, WCONT_T_SCALE, "scale-set-cb")) {
    return;
  }

  g_signal_connect (uiscale->uidata.widget, "change-value",
      G_CALLBACK (uiScaleChangeValueHandler), uicb);
}

double
uiScaleEnforceMax (uiwcont_t *uiscale, double value)
{
  GtkAdjustment   *adjustment;
  double          max;

  if (! uiwcontValid (uiscale, WCONT_T_SCALE, "scale-enforce-max")) {
    return value;
  }

  /* gtk scale's lower limit works, but upper limit is not respected */
  adjustment = gtk_range_get_adjustment (GTK_RANGE (uiscale->uidata.widget));
  max = gtk_adjustment_get_upper (adjustment);
  if (value > max) {
    value = max;
    gtk_adjustment_set_value (adjustment, value);
  }
  return value;
}

double
uiScaleGetValue (uiwcont_t *uiscale)
{
  double value = 0.0;

  if (! uiwcontValid (uiscale, WCONT_T_SCALE, "scale-get")) {
    return value;
  }

  value = gtk_range_get_value (GTK_RANGE (uiscale->uidata.widget));
  return value;
}

int
uiScaleGetDigits (uiwcont_t *uiscale)
{
  int value = 0;

  if (! uiwcontValid (uiscale, WCONT_T_SCALE, "scale-get-digits")) {
    return value;
  }

  value = gtk_scale_get_digits (GTK_SCALE (uiscale->uidata.widget));
  return value;
}

void
uiScaleSetValue (uiwcont_t *uiscale, double value)
{
  if (! uiwcontValid (uiscale, WCONT_T_SCALE, "scale-set")) {
    return;
  }

  gtk_range_set_value (GTK_RANGE (uiscale->uidata.widget), value);
}

void
uiScaleSetRange (uiwcont_t *uiscale, double start, double end)
{
  if (! uiwcontValid (uiscale, WCONT_T_SCALE, "scale-set-range")) {
    return;
  }

  gtk_range_set_range (GTK_RANGE (uiscale->uidata.widget), start, end);
}


/* internal routines */

/* the gtk documentation is incorrect.  scrolltype is not a pointer. */
static gboolean
uiScaleChangeValueHandler (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer udata)
{
  callback_t  *uicb = udata;
  bool        rc = UICB_CONT;

  if (! callbackIsSet (uicb)) {
    return rc;
  }

  rc = callbackHandlerD (uicb, value);
  return rc;
}
