/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uipbar.h"

uiwcont_t *
uiCreateProgressBar (void)
{
  uiwcont_t           *uiwidget;
  NSProgressIndicator *pbar;

  pbar = [[NSProgressIndicator alloc] init];

  uiwidget = uiwcontAlloc (WCONT_T_PROGRESS_BAR, WCONT_T_PROGRESS_BAR);
  uiwcontSetWidget (uiwidget, pbar, NULL);

  return uiwidget;
}

void
uiProgressBarSet (uiwcont_t *uipb, double val)
{
  NSProgressIndicator *pbar;

  pbar = uipb->widget;
  pbar.doubleValue = val;
}
