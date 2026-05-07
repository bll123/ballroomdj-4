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
#import <Foundation/NSDebug.h>

#include "oslocale.h"
#include "mdebug.h"
#include "tmutil.h"
#include "uiclass.h"
#include "uigeneral.h"

#include "ui/uiwcont-int.h"
#include "ui/uimacos-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"

uisetup_t   guisetup;

int uiBaseMarginSz = UIUTILS_BASE_MARGIN_SZ;

@interface AppDelegate : NSObject
- (void)keyDown : (NSEvent *)theEvent;
- (void)keyUp : (NSEvent *)theEvent;

- (BOOL)acceptsFirstResponder;
- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
@end

@implementation AppDelegate
- (void)keyDown : (NSEvent *)theEvent {}
- (void)keyUp : (NSEvent *)theEvent {}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
@end

bdj4uitype_t
uiBackend (void)
{
  return BDJ4_UI_ID_MACOS;
}

void
uiUIInitialize (int direction)
{
  id  appDelegate;

  [NSApplication sharedApplication];
  [NSApp setActivationPolicy : NSApplicationActivationPolicyRegular];
  [NSApp setPresentationOptions : NSApplicationPresentationDefault];
  [NSApp activateIgnoringOtherApps : YES];

  uiUIBasicInit ();

  appDelegate = [[AppDelegate alloc] init];
  [NSApp setDelegate : appDelegate];

  NSDebugEnabled = YES;
  NSZombieEnabled = YES;

  if (direction == TEXT_DIR_RTL) {
    ;
  }
  guisetup.direction = direction;
  return;
}

void
uiUIProcessEvents (void)
{
  NSEvent *currev = nil;
  bool    haveev = false;

#if MACOS_UI_DEBUG
  [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"NSConstraintBasedLayoutVisualizeMutuallyExclusiveConstraints"];
#endif

  while ((currev = [NSApp nextEventMatchingMask : NSEventMaskAny
        untilDate : [NSDate distantPast]
        inMode : NSDefaultRunLoopMode
        dequeue : YES])) {;
    [NSApp sendEvent : currev];
    haveev = true;
  }

  if (haveev) {
    [NSApp updateWindows];
  }
}

void
uiUIProcessWaitEvents (void)
{
  for (int i = 0; i < 4; ++i) {
    uiUIProcessEvents ();
    mssleep (5);
  }
  return;
}

void
uiCleanup (void)
{
  return;
}

void
uiSetUICSS (uisetup_t *uisetup)
{
  int       tdir;

  tdir = guisetup.direction;
  memcpy (&guisetup, uisetup, sizeof (uisetup_t));
  guisetup.direction = tdir;
  return;
}

void
uiAddColorClass (const char *classnm, const char *color)
{
  return;
}

void
uiAddBGColorClass (const char *classnm, const char *color)
{
  return;
}

void
uiAddProgressbarClass (const char *classnm, const char *color)
{
  return;
}

void
uiInitUILog (void)
{
  return;
}

void
uiwcontUIInit (uiwcont_t *uiwidget)
{
  macoslayout_t *layout;

  uiwidget->uidata = mdmalloc (sizeof (uispecific_t));
// fprintf (stderr, "ui-init: %s %d-%d\n", uiwcontDesc (uiwidget->wtype), uiwidget->ui_id, uiwidget->id);
  layout = &uiwidget->uidata->layout;

  /* default margins */
  /* top left bottom right */
  if (guisetup.direction == TEXT_DIR_RTL) {
    layout->margins = NSEdgeInsetsMake ((CGFloat) uiBaseMarginSz, 0.0, 0.0, (CGFloat) uiBaseMarginSz);
  } else {
    layout->margins = NSEdgeInsetsMake ((CGFloat) uiBaseMarginSz, (CGFloat) uiBaseMarginSz, 0.0, 0.0);
  }

  layout->container = NULL;
  layout->expandhoriz = false;
  layout->expandvert = false;
  layout->horizcenter = false;
  layout->horizend = false;
  layout->horizfill = false;
  layout->horizright = false;
  layout->horizstart = true;      /* default */
  layout->vertbaseline = false;
  layout->vertcenter = false;
  layout->vertend = false;
  layout->vertfill = false;
  layout->vertstart = true;       /* default */
}

/* on macos, every view is wrapped in a NSStackView so that the margins */
/* can be set using edge-insets */
void
uiwcontUIWidgetInit (uiwcont_t *uiwidget)
{
  macoslayout_t       *layout;
  NSView              *view;
//  NSLayoutConstraint  *c;

  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->wtype == WCONT_T_WINDOW) {
    return;
  }

  view = uiwidget->packwidget;
  layout = &uiwidget->uidata->layout;

  layout->container = [[NSStackView alloc] init];
  layout->container.spacing = 0.0;
  layout->container.edgeInsets = layout->margins;
  [layout->container addView : view inGravity : NSStackViewGravityCenter];

  [layout->container setHuggingPriority :
      NSLayoutPriorityRequired
      forOrientation : NSLayoutConstraintOrientationHorizontal];
  [layout->container setHuggingPriority :
      NSLayoutPriorityRequired
      forOrientation : NSLayoutConstraintOrientationVertical];

  [layout->container setContentHuggingPriority :
      NSLayoutPriorityDefaultHigh
      forOrientation : NSLayoutConstraintOrientationHorizontal];
  [layout->container setContentHuggingPriority :
      NSLayoutPriorityDefaultHigh
      forOrientation : NSLayoutConstraintOrientationVertical];

  /* otherwise everything gets shrunk to a minimal size */
  [layout->container setContentCompressionResistancePriority :
      NSLayoutPriorityRequired
      forOrientation : NSLayoutConstraintOrientationHorizontal];
  [layout->container setContentCompressionResistancePriority :
      NSLayoutPriorityRequired
      forOrientation : NSLayoutConstraintOrientationVertical];

// fprintf (stderr, "ui-widget-init: %s %d-%d\n", uiwcontDesc (uiwidget->wtype), uiwidget->ui_id, uiwidget->id);
  [view setContentHuggingPriority :
      NSLayoutPriorityDefaultHigh
      forOrientation : NSLayoutConstraintOrientationHorizontal];
  [view setContentHuggingPriority :
      NSLayoutPriorityDefaultHigh
      forOrientation : NSLayoutConstraintOrientationVertical];

  /* otherwise everything gets shrunk to a minimal size */
  [view setContentCompressionResistancePriority :
      NSLayoutPriorityRequired
      forOrientation : NSLayoutConstraintOrientationHorizontal];
  [view setContentCompressionResistancePriority :
      NSLayoutPriorityRequired
      forOrientation : NSLayoutConstraintOrientationVertical];

  view = uiwidget->widget;

  [view setContentHuggingPriority :
      NSLayoutPriorityDefaultHigh
      forOrientation : NSLayoutConstraintOrientationHorizontal];
  [view setContentHuggingPriority :
      NSLayoutPriorityDefaultHigh
      forOrientation : NSLayoutConstraintOrientationVertical];

  /* otherwise everything gets shrunk to a minimal size */
  [view setContentCompressionResistancePriority :
      NSLayoutPriorityRequired
      forOrientation : NSLayoutConstraintOrientationHorizontal];
  [view setContentCompressionResistancePriority :
      NSLayoutPriorityRequired
      forOrientation : NSLayoutConstraintOrientationVertical];

  return;
}

void
uiwcontUIFree (uiwcont_t *uiwidget)
{
  mdfree (uiwidget->uidata);
  return;
}
