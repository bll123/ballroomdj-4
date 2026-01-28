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

#include "uiwcont.h"

#include "ui/uiwcont-int.h"
#include "ui/uimacos-int.h"

#include "ui/uiui.h"
#include "ui/uilabel.h"

@implementation ILabel
- (BOOL) isFlipped {
  return YES;
}
@end

uiwcont_t *
uiCreateLabel (const char *label)
{
  uiwcont_t   *uiwidget;
  ILabel      *widget;

  widget = [[ILabel alloc] init];
  [widget setBezeled:NO];
  [widget setDrawsBackground:NO];
  [widget setEditable:NO];
  [widget setSelectable:NO];
  [widget setStringValue: [NSString stringWithUTF8String: label]];
  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

  uiwidget = uiwcontAlloc (WCONT_T_LABEL, WCONT_T_LABEL);
  uiwcontSetWidget (uiwidget, widget, NULL);
  return uiwidget;
}

void
uiLabelAddClass (const char *classnm, const char *color)
{
  return;
}

void
uiLabelSetTooltip (uiwcont_t *uiwidget, const char *txt)
{
//  ILabel *nstf;

  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-set-tooltip")) {
    return;
  }

// ### this will be very slow, as the old tool tips need to be removed.
// will probably be better to create a custom method and save the data
// for that label and use the dynamic methods.
// ### not working
//  nstf = uiwidget->uidata.widget;
//  [nstf addToolTip: [NSString stringWithUTF8String: txt]];
  return;
}

void
uiLabelSetFont (uiwcont_t *uiwidget, const char *font)
{
  return;
}

void
uiLabelSetText (uiwcont_t *uiwidget, const char *text)
{
  ILabel *widget;

  if (! uiwcontValid (uiwidget, WCONT_T_LABEL, "label-set-tooltip")) {
    return;
  }

  widget = uiwidget->uidata.widget;
  [widget setStringValue: [NSString stringWithUTF8String: text]];
  return;
}

const char *
uiLabelGetText (uiwcont_t *uiwidget)
{
  return NULL;
}

void
uiLabelEllipsizeOn (uiwcont_t *uiwidget)
{
  return;
}

void
uiLabelWrapOn (uiwcont_t *uiwidget)
{
  ILabel *widget;

  widget = uiwidget->uidata.widget;
  [widget setMaximumNumberOfLines:3];
  return;
}

void
uiLabelSetSelectable (uiwcont_t *uiwidget)
{
  ILabel *widget;

  widget = uiwidget->uidata.widget;
  [widget setSelectable:YES];
  return;
}

void
uiLabelSetMaxWidth (uiwcont_t *uiwidget, int width)
{
  return;
}

void
uiLabelAlignEnd (uiwcont_t *uiwidget)
{
  return;
}

void
uiLabelSetMinWidth (uiwcont_t *uiwidget, int width)
{
  return;
}
