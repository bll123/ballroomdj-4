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
uiutilsCreateProgressBar (char *color)
{
  GtkWidget *widget;
  char      tbuff [200];

  widget = gtk_progress_bar_new ();
  gtk_widget_set_halign (widget, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_widget_set_margin_start (widget, uiutilsBaseMarginSz);
  gtk_widget_set_margin_top (widget, uiutilsBaseMarginSz);
  snprintf (tbuff, sizeof (tbuff),
      "progress, trough { min-height: 25px; } progressbar > trough > progress { background-color: %s; }",
      color);
  uiutilsSetCss (widget, tbuff);
  return widget;
}

void
uiutilsProgressBarSet (GtkWidget *pb, double val)
{
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pb), val);
}
