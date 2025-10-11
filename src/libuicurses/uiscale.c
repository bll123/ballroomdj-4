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

#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiwidget.h"
#include "ui/uiscale.h"

uiwcont_t *
uiCreateScale (double lower, double upper,
    double stepinc, double pageinc, double initvalue, int digits)
{
  return NULL;
}

void
uiScaleSetCallback (uiwcont_t *uiscale, callback_t *uicb)
{
  return;
}

double
uiScaleEnforceMax (uiwcont_t *uiscale, double value)
{
  return 0.0;
}

double
uiScaleGetValue (uiwcont_t *uiscale)
{
  return 0.0;
}

int
uiScaleGetDigits (uiwcont_t *uiscale)
{
  return 0;
}

void
uiScaleSetValue (uiwcont_t *uiscale, double value)
{
  return;
}

void
uiScaleSetRange (uiwcont_t *uiscale, double start, double end)
{
  return;
}

