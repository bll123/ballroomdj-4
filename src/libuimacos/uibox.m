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

@interface IBox : NSStackView {}
- (BOOL) isFlipped;
@end

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

#if MACOS_UI_DEBUG

enum {
  MACOS_UI_DBG_WINDOW,
  MACOS_UI_DBG_EXP_CHILDREN,
  MACOS_UI_DBG_NORM,
  MACOS_UI_DBG_EXP_WIDTH,
  MACOS_UI_DBG_EXP_HEIGHT,
  MACOS_UI_DBG_COLS,
};

typedef struct dbgcol {
  double  r;
  double  g;
  double  b;
} dbgcol_t;

static dbgcol_t dbgcols [MACOS_UI_DBG_COLS] = {
  { 150.0 / 255.0,   0.0 / 255.0,   0.0 / 255.0 },
  {   0.0 / 255.0, 150.0 / 255.0,   0.0 / 255.0 },
  {   0.0 / 255.0,   0.0 / 255.0, 150.0 / 255.0 },
  { 150.0 / 255.0, 150.0 / 255.0,   0.0 / 255.0 },
  {   0.0 / 255.0, 150.0 / 255.0, 150.0 / 255.0 },
};

#endif

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
  [box addView : widget inGravity : grav];

  if (uibox->uidata.layout->expandchildren &&
      uibox->wtype == WCONT_T_VBOX) {
    [box.widthAnchor constraintEqualToAnchor :
        widget.widthAnchor].active = YES;
    widget.autoresizingMask |= NSViewWidthSizable;
#if MACOS_UI_DEBUG
    if (uiwidget->wbasetype == WCONT_T_BOX) {
fprintf (stderr, "c-box: %d pack into w\n", uiwidget->id);
      widget = uiwidget->uidata.widget;
      [[widget layer] setBorderColor :
          [NSColor colorWithRed : dbgcols [MACOS_UI_DBG_NORM].r
          green : dbgcols [MACOS_UI_DBG_NORM].g
          blue : dbgcols [MACOS_UI_DBG_NORM].b
          alpha:1.0].CGColor];
      widget.needsDisplay = YES;
    }
#endif
  }

  uiwidget->packed = true;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

/* this uses the GTK terminology */
/* expand allows any children to expand to fill the space */
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
  [box addView : widget inGravity : grav];

  if (uibox->uidata.layout->expandchildren &&
      uibox->wtype == WCONT_T_VBOX) {
    [box.widthAnchor constraintEqualToAnchor :
        widget.widthAnchor].active = YES;
    widget.autoresizingMask |= NSViewWidthSizable;
#if MACOS_UI_DEBUG
    if (uiwidget->wbasetype == WCONT_T_BOX) {
fprintf (stderr, "c-box: %d pack into w-e\n", uiwidget->id);
      widget = uiwidget->uidata.widget;
      [[widget layer] setBorderColor :
          [NSColor colorWithRed : dbgcols [MACOS_UI_DBG_EXP_WIDTH].r
          green : dbgcols [MACOS_UI_DBG_EXP_WIDTH].g
          blue : dbgcols [MACOS_UI_DBG_EXP_WIDTH].b
          alpha:1.0].CGColor];
      widget.needsDisplay = YES;
    }
#endif
  }

  if (uiwidget->wbasetype == WCONT_T_BOX) {
    uiwidget->uidata.layout->expandchildren = true;
  }

  uiwidget->packed = true;
  uiWidgetUpdateLayout (uiwidget);

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
  [box insertView : widget atIndex : 0 inGravity : grav];

  if (uibox->uidata.layout->expandchildren &&
      uibox->wtype == WCONT_T_HBOX) {
    [box.heightAnchor constraintEqualToAnchor :
        widget.heightAnchor].active = YES;
    widget.autoresizingMask |= NSViewHeightSizable;
#if MACOS_UI_DEBUG
    if (uiwidget->wbasetype == WCONT_T_BOX) {
fprintf (stderr, "c-box: %d pack into h\n", uiwidget->id);
      widget = uiwidget->uidata.widget;
      [[widget layer] setBorderColor :
          [NSColor colorWithRed : dbgcols [MACOS_UI_DBG_NORM].r
          green : dbgcols [MACOS_UI_DBG_NORM].g
          blue : dbgcols [MACOS_UI_DBG_NORM].b
          alpha:1.0].CGColor];
      widget.needsDisplay = YES;
    }
#endif
  }

  uiwidget->packed = true;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

/* this uses the GTK terminology */
/* expand allows any children to expand to fill the space */
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

  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityBottom;
  }

  [box insertView : widget atIndex : 0 inGravity : grav];

  if (uibox->uidata.layout->expandchildren &&
      uibox->wtype == WCONT_T_HBOX) {
    [box.heightAnchor constraintEqualToAnchor :
        widget.heightAnchor].active = YES;
    widget.autoresizingMask |= NSViewHeightSizable;
#if MACOS_UI_DEBUG
    if (uiwidget->wbasetype == WCONT_T_BOX) {
fprintf (stderr, "c-box: %d pack into h-e\n", uiwidget->id);
      widget = uiwidget->uidata.widget;
      [[widget layer] setBorderColor :
          [NSColor colorWithRed : dbgcols [MACOS_UI_DBG_EXP_HEIGHT].r
          green : dbgcols [MACOS_UI_DBG_EXP_HEIGHT].g
          blue : dbgcols [MACOS_UI_DBG_EXP_HEIGHT].b
          alpha:1.0].CGColor];
      widget.needsDisplay = YES;
    }
#endif
  }

  if (uiwidget->wbasetype == WCONT_T_BOX) {
    uiwidget->uidata.layout->expandchildren = true;
  }

  uiwidget->packed = true;
  uiWidgetUpdateLayout (uiwidget);

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
  [box setOrientation : orientation];
//  [box setTranslatesAutoresizingMaskIntoConstraints : NO];
  [box setDistribution : NSStackViewDistributionGravityAreas];
  box.autoresizingMask |= NSViewWidthSizable | NSViewHeightSizable;
  box.needsDisplay = true;
  box.spacing = 1.0;
  box.layerContentsRedrawPolicy = NSViewLayerContentsRedrawBeforeViewResize;

#if MACOS_UI_DEBUG
  [box setFocusRingType : NSFocusRingTypeExterior];
  [box setWantsLayer : YES];
  [[box layer] setBorderColor :
      [NSColor colorWithRed : dbgcols [MACOS_UI_DBG_WINDOW].r
      green : dbgcols [MACOS_UI_DBG_WINDOW].g
      blue : dbgcols [MACOS_UI_DBG_WINDOW].b
      alpha:1.0].CGColor];
  [[box layer] setBorderWidth : 2.0];
#endif

  if (orientation == NSUserInterfaceLayoutOrientationHorizontal) {
    uiwidget = uiwcontAlloc (WCONT_T_BOX, WCONT_T_HBOX);
    [box setAlignment : NSLayoutAttributeTop];
  }
  if (orientation == NSUserInterfaceLayoutOrientationVertical) {
    uiwidget = uiwcontAlloc (WCONT_T_BOX, WCONT_T_VBOX);
    [box setAlignment : NSLayoutAttributeLeading];
  }

  uiwcontSetWidget (uiwidget, box, NULL);
  uiwidget->uiint.uibox = uibox;

  [box setIdentifier :
      [[NSNumber numberWithUnsignedInt : uiwidget->id] stringValue]];

  return uiwidget;
}

