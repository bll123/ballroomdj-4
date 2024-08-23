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
#import <Foundation/NSDebug.h>

#include "oslocale.h"
#include "mdebug.h"
#include "tmutil.h"
#include "uiclass.h"
#include "uigeneral.h"

#include "ui/uiwcont-int.h"
#include "ui/uimacos-int.h"

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

  while ((currev = [NSApp nextEventMatchingMask: NSUIntegerMax
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

void
uiwcontUIInit (uiwcont_t *uiwidget)
{
  macosmargin_t *margins;

  if (uiwidget->uidata.margins != NULL) {
    return;
  }

  margins = mdmalloc (sizeof (macosmargin_t));
  uiwidget->uidata.margins = margins;

  margins->margins = NSEdgeInsetsMake (0, 0, 0, 0);
  margins->lguide = NULL;
}

void
uiwcontUIWidgetInit (uiwcont_t *uiwidget)
{
  NSView        *view = uiwidget->uidata.widget;
  macosmargin_t *margins = uiwidget->uidata.margins;

  if (uiwidget->wbasetype == WCONT_T_WINDOW) {
    return;
  }

  if (uiwidget->wbasetype == WCONT_T_BOX) {
    NSStackView *stackview = uiwidget->uidata.widget;

    stackview.edgeInsets = margins->margins;
    return;
  }

  margins->lguide = [[NSLayoutGuide alloc] init];
  [view addLayoutGuide: margins->lguide];

  if (margins->lguide == nil) {
    fprintf (stderr, "ERR: %d/%s %d/%s has nil anchor\n",
        uiwidget->wbasetype, uiwcontDesc (uiwidget->wbasetype),
        uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
    return;
  }

  [margins->lguide.leadingAnchor
      constraintEqualToAnchor: view.leadingAnchor
      constant: margins->margins.left].active = YES;
  [margins->lguide.trailingAnchor
      constraintEqualToAnchor: view.trailingAnchor
      constant: margins->margins.right].active = YES;
  [margins->lguide.topAnchor
      constraintEqualToAnchor: view.topAnchor
      constant: margins->margins.top].active = YES;
  [margins->lguide.bottomAnchor
      constraintEqualToAnchor: view.bottomAnchor
      constant: margins->margins.bottom].active = YES;
}

void
uiwcontUIFree (uiwcont_t *uiwidget)
{
  dataFree (uiwidget->uidata.margins);
}
