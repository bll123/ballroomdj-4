/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

#include "tmutil.h"
#include "uiclass.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"

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

  appDelegate = [[[AppDelegate alloc] init] autorelease];
  [NSApp setDelegate:appDelegate];

  return;
}

void
uiUIProcessEvents (void)
{
  NSEvent *currev = nil;
  bool    haveev = false;

  while ((currev = [NSApp nextEventMatchingMask: NSUIntegerMax
        untilDate: nil
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
uiSetUICSS (const char *uifont, const char *listingfont,
    const char *accentColor, const char *errorColor, const char *markColor,
    const char *selectColor, const char *rowhlColor)
{
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
