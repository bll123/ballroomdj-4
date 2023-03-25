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

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiadjustment.h"

uiwcont_t *
uiCreateAdjustment (double value, double start, double end,
    double stepinc, double pageinc, double pagesz)
{
  uiwcont_t       *uiadj;
  GtkAdjustment   *adjustment;

  uiadj = uiwcontAlloc ();
  uiadj->widget = NULL;

  adjustment = gtk_adjustment_new (value, start, end, stepinc, pageinc, pagesz);
  uiadj->adjustment = adjustment;
  return uiadj;
}

void *
uiAdjustmentGetAdjustment (uiwcont_t *uiadj)
{
  if (uiadj == NULL) {
    return NULL;
  }

  return uiadj->adjustment;
}
