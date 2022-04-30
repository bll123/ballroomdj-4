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

#include "log.h"
#include "uiutils.h"

GtkWidget *
uiutilsCreateScale (double lower, double upper,
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
  uiutilsSetCss (scale, "scale, trough { min-height: 5px; }");
  return scale;
}

double
uiutilsScaleEnforceMax (GtkWidget *scale, double value)
{
  GtkAdjustment   *adjustment;
  double          max;

  /* gtk scale's lower limit works, but upper limit is not respected */
  adjustment = gtk_range_get_adjustment (GTK_RANGE (scale));
  max = gtk_adjustment_get_upper (adjustment);
  if (value > max) {
    value = max;
    gtk_adjustment_set_value (adjustment, value);
  }
  return value;
}
