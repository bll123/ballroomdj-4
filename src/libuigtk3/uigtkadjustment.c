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

#include "ui/uiui.h"
#include "ui/uiadjustment.h"

void
uiCreateAdjustment (uiwcont_t *uiwidget, double value, double start, double end,
    double stepinc, double pageinc, double pagesz)
{
  GtkAdjustment     *adjustment;

  uiwidget->widget = NULL;
  if (uiwidget == NULL) {
    return;
  }

  adjustment = gtk_adjustment_new (value, start, end, stepinc, pageinc, pagesz);
  uiwidget->adjustment = adjustment;
}

void *
uiAdjustmentGetAdjustment (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return NULL;
  }

  return uiwidget->adjustment;
}
