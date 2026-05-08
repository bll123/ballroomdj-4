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
uiCreateChangeIndicator (void)
{
  uiwcont_t   *uiwidget;
  ILabel      *widget;

  widget = [[ILabel alloc] init];
  widget.bezeled = NO;
  widget.drawsBackground = NO;
  widget.editable = NO;
  widget.stringValue = [NSString stringWithUTF8String : " "];
  widget.translatesAutoresizingMaskIntoConstraints = NO;

  uiwidget = uiwcontAlloc (WCONT_T_CHGIND, WCONT_T_CHGIND);
  uiwcontSetWidget (uiwidget, widget, NULL);

  uiWidgetSetIdentifier (uiwidget, "chgind");

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

  widget = uiwidget->widget;
  col = [NSColor colorWithCalibratedWhite : 0.0 alpha : 0.0];
  widget.textColor = col;

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
  widget.textColor = col;

  return;
}
