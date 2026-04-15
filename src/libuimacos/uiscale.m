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

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiwidget.h"
#include "ui/uiscale.h"

uiwcont_t *
uiCreateScale (double lower, double upper,
    double stepinc, double pageinc, double initvalue, int digits)
{
  NSSlider    *widget;
  uiwcont_t   *uiwidget;

  widget = [NSSlider sliderWithValue : initvalue
      minValue : lower
      maxValue : upper
      target : nil
      action : nil];

  widget.altIncrementValue = stepinc;
  [widget setTranslatesAutoresizingMaskIntoConstraints : NO];
  widget.needsDisplay = true;

  uiwidget = uiwcontAlloc (WCONT_T_SCALE, WCONT_T_SCALE);
  uiwcontSetWidget (uiwidget, widget, NULL);

  [widget setIdentifier :
      [[NSNumber numberWithUnsignedInt : uiwidget->id] stringValue]];
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

