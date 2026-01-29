/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#include "colorutils.h"
#include "uigeneral.h"

#include "ui/uiwcont-int.h"
#include "ui/uimacos-int.h"

#include "ui/uiui.h"
#include "ui/uibox.h"
#include "ui/uiwidget.h"
#include "ui/uichgind.h"

uiwcont_t *
uiCreateChangeIndicator (uiwcont_t *boxp)
{
  uiwcont_t   *uiwidget;
  ILabel      *widget;

  widget = [[ILabel alloc] init];
  [widget setBezeled : NO];
  [widget setDrawsBackground : NO];
  [widget setEditable : NO];
  [widget setStringValue : [NSString stringWithUTF8String : " "]];
  [widget setTranslatesAutoresizingMaskIntoConstraints : NO];

  uiwidget = uiwcontAlloc (WCONT_T_CHGIND, WCONT_T_CHGIND);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiBoxPackStart (boxp, uiwidget);
  return uiwidget;
}

void
uichgindMarkNormal (uiwcont_t *uiwidget)
{
  ILabel      *widget;
  NSColor     *col;

  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-mark-norm")) {
    return;
  }

  widget = uiwidget->uidata.widget;
  col = [NSColor colorWithCalibratedWhite : 0.0 alpha : 0.0];
  [widget setTextColor : col];

  return;
}

void
uichgindMarkChanged (uiwcont_t *uiwidget)
{
  ILabel      *widget;
  NSColor     *col;
  double      r, g, b;

  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-mark-chg")) {
    return;
  }

  widget = uiwidget->uidata.widget;
  colorValues (guisetup.changedColor, &r, &g, &b);
  col = [NSColor colorWithCalibratedRed : r green : g blue : b alpha : 1.0];
  [widget setTextColor : col];

  return;
}
