#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "ui.h"
#include "uiutils.h"

static gboolean uiScaleChangeValueHandler (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata);

void
uiCreateScale (UIWidget *uiwidget, double lower, double upper,
    double stepinc, double pageinc, double initvalue)
{
  GtkWidget     *scale;
  GtkAdjustment *adjustment;

  adjustment = gtk_adjustment_new (0.0, lower, upper, stepinc, pageinc, 0.0);
  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);
  assert (scale != NULL);
  gtk_widget_set_size_request (scale, 200, 5);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_scale_set_has_origin (GTK_SCALE (scale), TRUE);
  gtk_range_set_value (GTK_RANGE (scale), initvalue);
  uiSetCss (scale, "scale, trough { min-height: 5px; }");
  uiwidget->widget = scale;
  uiWidgetSetMarginTop (uiwidget, uiBaseMarginSz);
  uiWidgetSetMarginStart (uiwidget, uiBaseMarginSz * 2);
}

void
uiScaleSetCallback (UIWidget *uiscale, UICallback *uicb)
{
  g_signal_connect (uiscale->widget, "change-value",
      G_CALLBACK (uiScaleChangeValueHandler), uicb);
}

double
uiScaleEnforceMax (UIWidget *uiscale, double value)
{
  GtkAdjustment   *adjustment;
  double          max;

  /* gtk scale's lower limit works, but upper limit is not respected */
  adjustment = gtk_range_get_adjustment (GTK_RANGE (uiscale->widget));
  max = gtk_adjustment_get_upper (adjustment);
  if (value > max) {
    value = max;
    gtk_adjustment_set_value (adjustment, value);
  }
  return value;
}

double
uiScaleGetValue (UIWidget *uiscale)
{
  double value;

  value = gtk_range_get_value (GTK_RANGE (uiscale->widget));
  return value;
}

void
uiScaleSetValue (UIWidget *uiscale, double value)
{
  gtk_range_set_value (GTK_RANGE (uiscale->widget), value);
}

void
uiScaleSetRange (UIWidget *uiscale, double start, double end)
{
  gtk_range_set_range (GTK_RANGE (uiscale->widget), start, end);
}

/* internal routines */

static gboolean
uiScaleChangeValueHandler (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata)
{
  UICallback  *uicb = udata;
  bool        rc = UICB_STOP;

  if (uicb == NULL) {
    return rc;
  }
  if (uicb->doublecb == NULL) {
    return rc;
  }

  rc = uicb->doublecb (uicb->udata, value);
  return rc;
}
