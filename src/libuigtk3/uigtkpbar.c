/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
  uiwidget->wbasetype = WCONT_T_PROGRESS_BAR;
  uiwidget->wtype = WCONT_T_PROGRESS_BAR;
  uiwidget->widget = widget;
  return uiwidget;
}

void
uiProgressBarSet (uiwcont_t *uipb, double val)
{
  if (! uiwcontValid (uipb, WCONT_T_PROGRESS_BAR, "pbar-set")) {
    return;
  }

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (uipb->widget), val);
}
