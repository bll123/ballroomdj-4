/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uibox.h"
#include "ui/uiwidget.h"
#include "ui/uichgind.h"

@interface ILabel : NSTextField {}
- (BOOL) isFlipped;
@end

@implementation ILabel
- (BOOL) isFlipped {
  return YES;
}
@end

uiwcont_t *
uiCreateChangeIndicator (uiwcont_t *boxp)
{
  uiwcont_t   *uiwidget;
  ILabel      *widget;

fprintf (stderr, "c-chgind\n");
  widget = [[ILabel alloc] init];
  [widget setBezeled:NO];
  [widget setDrawsBackground:NO];
  [widget setEditable:NO];
  [widget setStringValue: [NSString stringWithUTF8String: " "]];
  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

  uiwidget = uiwcontAlloc (WCONT_T_LABEL, WCONT_T_LABEL);
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
  col = [NSColor colorWithCalibratedWhite:0.0 alpha:0.0];
  [widget setTextColor: col];

  return;
}

void
uichgindMarkChanged (uiwcont_t *uiwidget)
{
  ILabel      *widget;
  NSColor     *col;

  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-mark-chg")) {
    return;
  }

  widget = uiwidget->uidata.widget;
  // ### this needs access to the colors
  col = [NSColor colorWithCalibratedRed:0.0 green:0.8 blue:0.0 alpha:1.0];
  [widget setTextColor: col];

  return;
}
