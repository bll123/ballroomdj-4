/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
  uiadj->wbasetype = WCONT_T_ADJUSTMENT;
  uiadj->wtype = WCONT_T_ADJUSTMENT;
  uiadj->uidata.widget = NULL;

  adjustment = gtk_adjustment_new (value, start, end, stepinc, pageinc, pagesz);
  uiadj->uidata.adjustment = adjustment;
  return uiadj;
}

void *
uiAdjustmentGetAdjustment (uiwcont_t *uiadj)
{
  if (! uiwcontValid (uiadj, WCONT_T_ADJUSTMENT, "adj-get")) {
    return NULL;
  }

  return uiadj->uidata.adjustment;
}
