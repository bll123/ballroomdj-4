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

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"

static uiwcont_t * uiCreateBox (int orientation);

uiwcont_t *
uiCreateVertBox (void)
{
  return uiCreateBox (NSUserInterfaceLayoutOrientationVertical);
}

uiwcont_t *
uiCreateHorizBox (void)
{
  return uiCreateBox (NSUserInterfaceLayoutOrientationHorizontal);
}

void
uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  NSStackView *box;
  NSView      *widget = NULL;
  int         grav = NSStackViewGravityLeading;

  if (uibox == NULL || uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityTop;
  }
  [box addView: widget inGravity: grav];
  return;
}

void
uiBoxPackStartExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  NSStackView    *box;
  NSView        *widget = NULL;
  int         grav = NSStackViewGravityLeading;

  if (uibox == NULL || uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityTop;
  }
  [box addView: widget inGravity: grav];
  return;
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  NSStackView    *box;
  NSView        *widget = NULL;
  int         grav = NSStackViewGravityTrailing;

  if (uibox == NULL || uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityBottom;
  }
  [box addView: widget inGravity: grav];
  return;
}

void
uiBoxPackEndExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  NSStackView    *box;
  NSView        *widget = NULL;
  int         grav = NSStackViewGravityTrailing;

  if (uibox == NULL || uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityBottom;
  }
  [box addView: widget inGravity: grav];
  return;
}

void
uiBoxSetSizeChgCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  return;
}

/* internal routines */

static uiwcont_t *
uiCreateBox (int orientation)
{
  uiwcont_t   *uiwidget;
  NSStackView  *box = NULL;

  box = [[NSStackView alloc] init];
  [box setOrientation: orientation];
  [box setTranslatesAutoresizingMaskIntoConstraints: NO];

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_BOX;
  uiwidget->wtype = WCONT_T_VBOX;
  if (orientation == NSUserInterfaceLayoutOrientationHorizontal) {
    uiwidget->wtype = WCONT_T_HBOX;
{
NSTextField *l;
int         grav = NSStackViewGravityLeading;

l = [[NSTextField alloc] init];
[l setBezeled:NO];
[l setDrawsBackground:NO];
[l setEditable:NO];
[l setStringValue: @"h"];
[box addView: l inGravity: grav];
}
    } else {
{
NSTextField *l;
int         grav = NSStackViewGravityTop;

l = [[NSTextField alloc] init];
[l setBezeled:NO];
[l setDrawsBackground:NO];
[l setEditable:NO];
[l setStringValue: @"v"];
[box addView: l inGravity: grav];
}
    }
  uiwidget->wtype = WCONT_T_VBOX;
  uiwidget->uidata.widget = box;
  uiwidget->uidata.packwidget = box;
  return uiwidget;
}

