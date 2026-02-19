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
- (void)keyDown:(NSEvent *)theEvent;
- (void)keyUp:(NSEvent *)theEvent;

- (BOOL)acceptsFirstResponder;
- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
@end

@implementation AppDelegate
- (void)keyDown:(NSEvent *)theEvent {}
- (void)keyUp:(NSEvent *)theEvent {}

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
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  [NSApp setPresentationOptions:NSApplicationPresentationDefault];
  [NSApp activateIgnoringOtherApps:YES];

  appDelegate = [[AppDelegate alloc] init];
  [NSApp setDelegate:appDelegate];

  NSDebugEnabled = YES;
  NSZombieEnabled = YES;

//  if (direction == TEXT_DIR_RTL) {
//    [NSApp userInterfaceLayoutDirection:rightToLeft];
//  }
  return;
}

void
uiUIProcessEvents (void)
{
  NSEvent *currev = nil;
  bool    haveev = false;

  while ((currev = [NSApp nextEventMatchingMask: NSEventMaskAny
        untilDate: [NSDate distantPast]
        inMode: NSDefaultRunLoopMode
        dequeue: YES])) {;
    [NSApp sendEvent:currev];
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
  memcpy (&guisetup, uisetup, sizeof (uisetup_t));
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

  layout->margins = NSEdgeInsetsMake (0, 0, 0, 0);
  layout->container = NULL;
  layout->centered = false;
  layout->expandhoriz = false;
  layout->expandvert = false;
  layout->alignright = false;
}

void
uiwcontUIWidgetInit (uiwcont_t *uiwidget)
{
  NSView        *view = uiwidget->uidata.widget;
  macoslayout_t *layout = uiwidget->uidata.layout;

  if (uiwidget->wbasetype == WCONT_T_WINDOW) {
    return;
  }

  layout->container = [[NSStackView alloc] init];
  [layout->container addView: view inGravity: NSStackViewGravityLeading];

  if (uiwidget->uidata.widget == uiwidget->uidata.packwidget) {
    uiwidget->uidata.packwidget = layout->container;
  }
}

void
uiwcontUIFree (uiwcont_t *uiwidget)
{
  return;
}
