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
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uipbar.h"

uiwcont_t *
uiCreateProgressBar (void)
{
  uiwcont_t *uiwidget;
  GtkWidget *widget;

  widget = gtk_progress_bar_new ();
  gtk_widget_set_halign (widget, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  uiwidget = uiwcontAlloc ();
  uiwidget->widget = widget;
  return uiwidget;
}

void
uiProgressBarSet (uiwcont_t *uipb, double val)
{
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (uipb->widget), val);
}
