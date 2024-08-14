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

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uilabel.h"

uiwcont_t *
uiCreateLabel (const char *label)
{
  uiwcont_t   *uiwidget;
  NSTextField *widget;

fprintf (stderr, "box: create label '%s'\n", label);
  widget = [[NSTextField alloc] init];
  [widget setBezeled:NO];
  [widget setDrawsBackground:NO];
  [widget setEditable:NO];
  [widget setStringValue: [NSString stringWithUTF8String: label]];
//  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
//  gtk_widget_set_margin_start (widget, uiBaseMarginSz);

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_LABEL;
  uiwidget->wtype = WCONT_T_LABEL;
  uiwidget->uidata.widget = widget;
  uiwidget->uidata.packwidget = widget;
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
  NSTextField *widget = uiwidget->uidata.widget;

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
  return;
}

void
uiLabelSetSelectable (uiwcont_t *uiwidget)
{
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
