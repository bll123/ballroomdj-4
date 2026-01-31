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

const char *
uiBackend (void)
{
  return "macos";
}

void
uiUIInitialize (int direction)
{
  id  appDelegate;

  [NSApplication sharedApplication];
  [NSApp setActivationPolicy : NSApplicationActivationPolicyRegular];
  [NSApp setPresentationOptions : NSApplicationPresentationDefault];
  [NSApp activateIgnoringOtherApps : YES];

  appDelegate = [[AppDelegate alloc] init];
  [NSApp setDelegate : appDelegate];

  NSDebugEnabled = YES;
  NSZombieEnabled = YES;

//  if (direction == TEXT_DIR_RTL) {
//    [NSApp userInterfaceLayoutDirection : rightToLeft];
//  }
  guisetup.direction = direction;
  return;
}

void
uiUIProcessEvents (void)
{
  NSEvent *currev = nil;
  bool    haveev = false;

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

  if (uiwidget->uidata.layout != NULL) {
    return;
  }

  layout = mdmalloc (sizeof (macoslayout_t));
  uiwidget->uidata.layout = layout;

  /* default margins */
  /* top left bottom right */
  if (guisetup.direction == TEXT_DIR_RTL) {
    layout->margins = NSEdgeInsetsMake ((CGFloat) uiBaseMarginSz, 0.0, 0.0, (CGFloat) uiBaseMarginSz);
  } else {
    layout->margins = NSEdgeInsetsMake ((CGFloat) uiBaseMarginSz, (CGFloat) uiBaseMarginSz, 0.0, 0.0);
  }

  layout->container = NULL;
  layout->expandchildren = false;
}

/* on macos, every view is wrapped in a NSStackView so that the margins */
/* can be set.  */
/* Supposedly a layoutguide can be used as a container, but */
/* it is not a view, and I'm not sure how to make it work */
void
uiwcontUIWidgetInit (uiwcont_t *uiwidget)
{
  macoslayout_t *layout;

  if (uiwidget->wbasetype == WCONT_T_WINDOW) {
    return;
  }

  layout = uiwidget->uidata.layout;

  /* on macos, there are standard images, and an image view */
  if (uiwidget->wtype != WCONT_T_IMAGE) {
    NSView        *view;

    view = uiwidget->uidata.widget;

    layout->container = [[NSStackView alloc] init];
    layout->container.spacing = 0.0;
    layout->container.edgeInsets = layout->margins;
    [layout->container addView : view inGravity : NSStackViewGravityCenter];
//    layout->container.needsDisplay = YES;
//    view.needsDisplay = YES;

    [layout->container setHuggingPriority :
        NSLayoutPriorityRequired
        forOrientation : NSLayoutConstraintOrientationHorizontal];
    [layout->container setHuggingPriority :
        NSLayoutPriorityRequired
        forOrientation : NSLayoutConstraintOrientationVertical];
    [layout->container setContentHuggingPriority :
        NSLayoutPriorityRequired
        forOrientation : NSLayoutConstraintOrientationHorizontal];
    [layout->container setContentHuggingPriority :
        NSLayoutPriorityRequired
        forOrientation : NSLayoutConstraintOrientationVertical];
    /* otherwise everything gets shrunk to a minimal size */
    [layout->container setContentCompressionResistancePriority :
        NSLayoutPriorityRequired
        forOrientation : NSLayoutConstraintOrientationHorizontal];
    [layout->container setContentCompressionResistancePriority :
        NSLayoutPriorityRequired
        forOrientation : NSLayoutConstraintOrientationVertical];

    [view setContentHuggingPriority :
        NSLayoutPriorityRequired
        forOrientation : NSLayoutConstraintOrientationHorizontal];
    [view setContentHuggingPriority :
        NSLayoutPriorityRequired
        forOrientation : NSLayoutConstraintOrientationVertical];

    /* otherwise everything gets shrunk to a minimal size */
    [view setContentCompressionResistancePriority :
        NSLayoutPriorityRequired
        forOrientation : NSLayoutConstraintOrientationHorizontal];
    [view setContentCompressionResistancePriority :
        NSLayoutPriorityRequired
        forOrientation : NSLayoutConstraintOrientationVertical];

//    view.autoresizingMask |= NSViewWidthSizable | NSViewHeightSizable;
    layout->container.autoresizingMask |= NSViewWidthSizable | NSViewHeightSizable;
  }

// ### this situation needs to be handled
  if (uiwidget->uidata.packwidget == uiwidget->uidata.widget) {
    uiwidget->uidata.packwidget = layout->container;
  }

  return;
}

void
uiwcontUIFree (uiwcont_t *uiwidget)
{
  return;
}
