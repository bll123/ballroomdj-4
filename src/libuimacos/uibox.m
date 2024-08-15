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

@interface IBox : NSStackView {}
- (BOOL) isFlipped;
@end

@implementation IBox
- (BOOL) isFlipped {
  return YES;
}
@end

#include "uigeneral.h"    // debug flag
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uiwidget.h"

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
  IBox    *box;
  NSView  *widget = NULL;
  int     grav = NSStackViewGravityLeading;

  if (uibox == NULL || uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityTop;
  }
  [box addView: widget inGravity: grav];
  uiWidgetSetMarginTop (uiwidget, 1);
  uiWidgetSetMarginStart (uiwidget, 1);
  uiwidget->packed = true;
  return;
}

void
uiBoxPackStartExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox    *box;
  NSView  *widget = NULL;
  int     grav = NSStackViewGravityLeading;

  if (uibox == NULL || uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  [box addView: widget inGravity: grav];
  uiWidgetSetMarginTop (uiwidget, 1);
  uiWidgetSetMarginStart (uiwidget, 1);
  uiwidget->packed = true;
  return;
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox        *box;
  NSView      *widget = NULL;
  int         grav = NSStackViewGravityTrailing;

  if (uibox == NULL || uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityBottom;
  }
  [box insertView: widget atIndex: 0 inGravity: grav];
  uiWidgetSetMarginTop (uiwidget, 1);
  uiWidgetSetMarginStart (uiwidget, 1);
  uiwidget->packed = true;
  return;
}

void
uiBoxPackEndExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox        *box;
  NSView      *widget = NULL;
  int         grav = NSStackViewGravityTrailing;

  if (uibox == NULL || uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  [box insertView: widget atIndex: 0 inGravity: grav];
  uiWidgetSetMarginTop (uiwidget, 1);
  uiWidgetSetMarginStart (uiwidget, 1);
  uiwidget->packed = true;
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
  IBox        *box = NULL;

  box = [[IBox alloc] init];
  [box setOrientation: orientation];
  [box setTranslatesAutoresizingMaskIntoConstraints: NO];
  box.spacing = 1.0;

#if MACOS_UI_DEBUG
  [box setFocusRingType: NSFocusRingTypeExterior];
  [box setWantsLayer: YES];
  [[box layer] setBorderWidth: 2.0];
#endif

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_BOX;

  if (orientation == NSUserInterfaceLayoutOrientationHorizontal) {
    uiwidget->wtype = WCONT_T_HBOX;
    [box setAlignment: NSLayoutAttributeTop];
  }
  if (orientation == NSUserInterfaceLayoutOrientationVertical) {
    uiwidget->wtype = WCONT_T_VBOX;
    [box setAlignment: NSLayoutAttributeLeading];
  }
  uiwidget->uidata.widget = box;
  uiwidget->uidata.packwidget = box;
  return uiwidget;
}

