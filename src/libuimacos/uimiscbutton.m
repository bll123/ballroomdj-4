/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uimiscbutton.h"

uiwcont_t *
uiCreateFontButton (const char *fontname)
{
fprintf (stderr, "c-f-bt\n");
  return NULL;
}

const char *
uiFontButtonGetFont (uiwcont_t *uiwidget)
{
  return NULL;
}

uiwcont_t *
uiCreateColorButton (const char *ident, const char *colortxt)
{
  NSColorWell     *widget;
  NSColor         *color;
  uiwcont_t       *uiwidget;
  int             r, g, b;

fprintf (stderr, "c-col-bt\n");
  widget = [[NSColorWell alloc] init];
  [widget setIdentifier : [NSString stringWithUTF8String : ident]];
//  [widget setAction : @selector(OnButton1Click : )];
  [widget setTranslatesAutoresizingMaskIntoConstraints : NO];

  sscanf (colortxt, "#%2x%2x%2x", &r, &g, &b);
  color = [NSColor colorWithRed : (CGFloat) r
      green : (CGFloat) g
      blue : (CGFloat) b
      alpha : 1.0];
  widget.color = color;
  widget.needsDisplay = true;

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON, WCONT_T_COLOR_BUTTON);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwcontSetIdent (uiwidget, ident);

  return NULL;
}

void
uiColorButtonGetColor (uiwcont_t *uiwidget, char *tbuff, size_t sz)
{
  return;
}
