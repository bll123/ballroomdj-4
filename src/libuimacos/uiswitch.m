/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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
#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiimage.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uiswitch.h"

typedef struct uiswitch {
  int       junk;
} uiswitch_t;

uiwcont_t *
uiCreateSwitch (int value)
{
  uiwcont_t   *uiwidget;
  NSSwitch    *widget = nil;

//  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
//  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  widget = [[NSSwitch alloc] init];
  [widget setState: NSControlStateValueOff];
  if (value) {
    [widget setState: NSControlStateValueOn];
  }
  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_SWITCH;
  uiwidget->wtype = WCONT_T_SWITCH;
  uiwidget->uidata.widget = widget;
  uiwidget->uidata.packwidget = widget;

  return uiwidget;
}

void
uiSwitchFree (uiwcont_t *uiwidget)
{
  return;
}

void
uiSwitchSetValue (uiwcont_t *uiwidget, int value)
{
  return;
}

int
uiSwitchGetValue (uiwcont_t *uiwidget)
{
  return 0;
}

void
uiSwitchSetCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}
