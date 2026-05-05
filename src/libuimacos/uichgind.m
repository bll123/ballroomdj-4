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

#include "ui/uibox.h"
#include "ui/uichgind.h"
#include "ui/uigrid.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

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
  widget.translatesAutoresizingMaskIntoConstraints = NO;

  uiwidget = uiwcontAlloc (WCONT_T_CHGIND, WCONT_T_CHGIND);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiBoxPackStart (boxp, uiwidget, WCONT_KEEP);

  uiWidgetSetIdentifier (uiwidget, "chgind");

  return uiwidget;
}

void
uichgindPackGrid (uiwcont_t *uiwidget, uiwcont_t *grid)
{
  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-pack-grid")) {
    return;
  }
  if (! uiwcontValid (grid, WCONT_T_GRID, "ci-pack-grid")) {
    return;
  }

  uiGridPack (grid, uiwidget, WCONT_KEEP);
}

void
uichgindMarkNormal (uiwcont_t *uiwidget)
{
  ILabel      *widget;
  NSColor     *col;

  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-mark-norm")) {
    return;
  }

  widget = uiwidget->widget;
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

  widget = uiwidget->widget;
  colorValues (guisetup.changedColor, &r, &g, &b);
  col = [NSColor colorWithCalibratedRed : r green : g blue : b alpha : 1.0];
  [widget setTextColor : col];

  return;
}
