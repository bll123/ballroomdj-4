/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
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

#include "mdebug.h"
#include "uigeneral.h"    // debug flag
#include "uiwcont.h"

#include "ui/uiwcont-int.h"
#include "ui/uimacos-int.h"

#include "ui/uibox.h"
#include "ui/uiwidget.h"

@implementation IBox
- (BOOL) isFlipped {
  return YES;
}
@end

typedef struct uibox {
  uiwcont_t   *priorstart;
  uiwcont_t   *priorend;
} uibox_t;

static uiwcont_t * uiCreateBox (int orientation);

uiwcont_t *
uiCreateVertBox (void)
{
fprintf (stderr, "c-vbox\n");
  return uiCreateBox (NSUserInterfaceLayoutOrientationVertical);
}

uiwcont_t *
uiCreateHorizBox (void)
{
fprintf (stderr, "c-hbox\n");
  return uiCreateBox (NSUserInterfaceLayoutOrientationHorizontal);
}

void
uiBoxFree (uiwcont_t *uibox)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-free")) {
    return;
  }
}

void
uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox          *box;
  NSView        *widget = NULL;
  int           grav = NSStackViewGravityLeading;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
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
  IBox          *box;
  NSView        *widget = NULL;
  int           grav = NSStackViewGravityLeading;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
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
  IBox          *box;
  NSView        *widget = NULL;
  int           grav = NSStackViewGravityTrailing;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
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
  IBox          *box;
  NSView        *widget = NULL;
  int           grav = NSStackViewGravityTrailing;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
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
  if (! uiwcontValid (uiwindow, WCONT_T_BOX, "box-set-size-chg-cb")) {
    return;
  }

  return;
}

/* internal routines */

static uiwcont_t *
uiCreateBox (int orientation)
{
  uiwcont_t   *uiwidget = NULL;
  uibox_t     *uibox = NULL;
  IBox        *box = NULL;

  uibox = mdmalloc (sizeof (uibox_t));
  uibox->priorstart = NULL;
  uibox->priorend = NULL;

  box = [[IBox alloc] init];
  [box setOrientation: orientation];
  [box setTranslatesAutoresizingMaskIntoConstraints: NO];
  [box setDistribution: NSStackViewDistributionGravityAreas];
  box.spacing = 1.0;

#if MACOS_UI_DEBUG
  [box setFocusRingType: NSFocusRingTypeExterior];
  [box setWantsLayer: YES];
  [[box layer] setBorderWidth: 2.0];
#endif

  if (orientation == NSUserInterfaceLayoutOrientationHorizontal) {
    uiwidget = uiwcontAlloc (WCONT_T_BOX, WCONT_T_HBOX);
    [box setAlignment: NSLayoutAttributeTop];
  }
  if (orientation == NSUserInterfaceLayoutOrientationVertical) {
    uiwidget = uiwcontAlloc (WCONT_T_BOX, WCONT_T_VBOX);
    [box setAlignment: NSLayoutAttributeLeading];
  }
  uiwcontSetWidget (uiwidget, box, NULL);
  uiwidget->uiint.uibox = uibox;

  return uiwidget;
}

