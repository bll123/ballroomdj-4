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

#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"
#include "ui/uimacos-int.h"

#include "ui/uiui.h"
#include "ui/uibox.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

@interface IWindow : NSWindow { }
@property callback_t  *closecb;
@property callback_t  *doubleclickcb;
- (instancetype) init;
- (void) setCloseCallback : (callback_t *) tcb;
- (void) setDoubleClickCallback : (callback_t *) tcb;
- (void) awakeFromNib;
- (BOOL) isFlipped;
@end

@implementation IWindow

- (instancetype) init {
  [super initWithContentRect : NSMakeRect (10, 10, 100, 100)
      styleMask : NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
          NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable |
          NSWindowStyleMaskFullSizeContentView
      backing : NSBackingStoreBuffered
      defer : NO];
  [self setIsVisible : YES];
  self.closecb = NULL;
  self.doubleclickcb = NULL;
  return self;
}

- (void) setCloseCallback : (callback_t *) tcb {
  self.closecb = tcb;
}

- (void) setDoubleClickCallback : (callback_t *) tcb {
  self.doubleclickcb = tcb;
}

- (void) awakeFromNib {
//  IWindow*  w = self;
//  NSStackView *box;

//  box = [w contentView];
}

- (BOOL) isFlipped {
  return YES;
}

@end

@implementation IWindowDelegate : NSObject

- (void) windowDidBecomeMain : (NSNotification *)notification {
  NSLog (@"Window : become main");
}

- (void) windowDidResignMain : (NSNotification *)notification {
  NSLog  (@"Window : resign main");
}

- (void) windowDidBecomeKey : (NSNotification *)notification {
  NSLog (@"Window : become key");
}

- (void) windowDidResignKey : (NSNotification *)notification {
  NSLog (@"Window : resign key");
}

- (BOOL) canBecomeKeyWindow {
  // Required for NSWindowStyleMaskBorderless windows
  return YES;
}

- (BOOL) canBecomeMainWindow {
  return YES;
}

- (IBAction) OnButton1Click : (id) sender {
NSLog (@"Window : button-1");
}

- (IBAction) OnButton2Click : (id) sender {
//  IWindow *window = sender.object;
NSLog (@"Window : button-2");
//  if (window.doubleclickcb != NULL) {
//    callbackHandler (window.doubleclickcb);
//  }
}

// This will close/terminate the application when the main window is closed.
- (void)windowWillClose : (NSNotification *)notification {
  IWindow *window = notification.object;
NSLog (@"Window : closing");
  if (window.closecb != NULL) {
NSLog (@"Window : call close-cb");
    callbackHandler (window.closecb);
  }
  if (window.isMainWindow) {
    [NSApp terminate : nil];
  }
}

@end

uiwcont_t *
uiCreateMainWindow (callback_t *uicb, const char *title, const char *imagenm)
{
  uiwcont_t     *uiwin;
  IWindow       *win = NULL;
  uiwcont_t     *uibox;
  NSStackView   *box;
  id            windowDelegate;

  win = [[IWindow alloc] init];
  uibox = uiCreateVertBox ();
  if (title != NULL) {
    NSString  *nstitle;

    nstitle = [NSString stringWithUTF8String : title];
    [win setTitle : nstitle];
  }

  box = uibox->uidata.widget;
  box.autoresizingMask |= NSViewWidthSizable | NSViewHeightSizable;

  [win setContentView : box];
  [win makeMainWindow];

  uibox->packed = true;
  uibox->uidata.layout->expandchildren = true;
  [box setIdentifier :
      [[NSNumber numberWithUnsignedInt : uibox->id] stringValue]];

  if (imagenm != NULL) {
    NSImage *image = nil;

    NSString *ns = [NSString stringWithUTF8String : imagenm];
    image = [[NSImage alloc] initWithContentsOfFile : ns];
    [[NSApplication sharedApplication] setApplicationIconImage : image];
    [image release];
  }

  windowDelegate = [[IWindowDelegate alloc] init];
  [win setDelegate : windowDelegate];

  [win setCloseCallback : uicb];

  uiwin = uiwcontAlloc (WCONT_T_WINDOW, WCONT_T_WINDOW);
  uiwcontSetWidget (uiwin, win, NULL);
  uiwin->packed = true;

  [win setIdentifier :
      [[NSNumber numberWithUnsignedInt : uiwin->id] stringValue]];

  uiWidgetSetAllMargins (uibox, 2);

  return uiwin;
}

void
uiWindowSetTitle (uiwcont_t *uiwindow, const char *title)
{
  IWindow   *win = NULL;
  NSString  *nstitle = NULL;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-title")) {
    return;
  }
  if (title == NULL) {
    return;
  }

  nstitle = [NSString stringWithUTF8String : title];

  win = uiwindow->uidata.widget;
  [win setTitle : nstitle];
  return;
}

void
uiCloseWindow (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-close")) {
    return;
  }

  return;
}

bool
uiWindowIsMaximized (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-is-max")) {
    return false;
  }

  return false;
}

void
uiWindowIconify (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-iconify")) {
    return;
  }

  return;
}

void
uiWindowDeIconify (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-deiconify")) {
    return;
  }

  return;
}

void
uiWindowMaximize (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-maximize")) {
    return;
  }

  return;
}

void
uiWindowUnMaximize (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-unmaximize")) {
    return;
  }

  return;
}

void
uiWindowDisableDecorations (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-disable-dec")) {
    return;
  }

  return;
}

void
uiWindowEnableDecorations (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-enable-dec")) {
    return;
  }

  return;
}

void
uiWindowGetSize (uiwcont_t *uiwindow, int *x, int *y)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-get-sz")) {
    return;
  }

  return;
}

void
uiWindowSetDefaultSize (uiwcont_t *uiwindow, int x, int y)
{
  NSWindow  *win;
  NSSize    nssz;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-dflt-sz")) {
    return;
  }

  win = uiwindow->uidata.widget;
  nssz.width = (CGFloat) x;
  nssz.height = (CGFloat) y;
  [win setContentSize : nssz];

  return;
}

void
uiWindowGetPosition (uiwcont_t *uiwindow, int *x, int *y, int *ws)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-get-pos")) {
    return;
  }

  return;
}

void
uiWindowMove (uiwcont_t *uiwindow, int x, int y, int ws)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-move")) {
    return;
  }

  return;
}

void
uiWindowMoveToCurrentWorkspace (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-move-curr-ws")) {
    return;
  }

  return;
}

void
uiWindowNoFocusOnStartup (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-no-focus-startup")) {
    return;
  }

  return;
}

uiwcont_t *
uiCreateScrolledWindow (int minheight)
{
  uiwcont_t     *uiscwin;
  NSScrollView  *win = NULL;

  win = [[NSScrollView alloc] init];
  win.autohidesScrollers = YES;
  win.hasVerticalScroller = YES;
  win.hasHorizontalScroller = NO;
  win.autoresizingMask |= NSViewWidthSizable | NSViewHeightSizable;

  uiscwin = uiwcontAlloc (WCONT_T_WINDOW, WCONT_T_SCROLL_WINDOW);
  uiwcontSetWidget (uiscwin, win, NULL);

  [win setIdentifier :
      [[NSNumber numberWithUnsignedInt : uiscwin->id] stringValue]];
  return uiscwin;
}

void
uiWindowSetPolicyExternal (uiwcont_t *uisw)
{
  if (! uiwcontValid (uisw, WCONT_T_WINDOW, "win-set-policy-ext")) {
    return;
  }

  return;
}

uiwcont_t *
uiCreateDialogWindow (uiwcont_t *parentwin,
    uiwcont_t *attachment, callback_t *uicb, const char *title)
{
  if (! uiwcontValid (parentwin, WCONT_T_WINDOW, "win-create-dialog-win")) {
    return NULL;
  }

  return NULL;
}

void
uiWindowSetDoubleClickCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  IWindow  *win;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-dclick-cb")) {
    return;
  }

  win = uiwindow->uidata.widget;
  [win setCloseCallback : uicb];

  return;
}

void
uiWindowSetWinStateCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-state-cb")) {
    return;
  }

  return;
}

void
uiWindowNoDim (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-no-dim")) {
    return;
  }

  return;
}

void
uiWindowPresent (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-present")) {
    return;
  }

  return;
}

void
uiWindowRaise (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-raise")) {
    return;
  }

  return;
}

void
uiWindowFind (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-find")) {
    return;
  }

  return;
}

void
uiWindowSetNoMaximize (uiwcont_t *uiwindow)
{
//  int      sm;
//  IWindow  *win;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-nomax")) {
    return;
  }

//  win = uiwindow->uidata.widget;
//  sm = win.styleMask;
//  sm &= ~NSWindowStyleMaskResizable;
//  win.styleMask = sm;
  return;
}

void
uiWindowPackInWindow (uiwcont_t *uiwindow, uiwcont_t *uiwidget)
{
  NSStackView   *container = NULL;
  NSView        *widget = NULL;
  NSStackView   *winbox = NULL;
  int           grav = NSStackViewGravityTop;
  macoslayout_t *layout = NULL;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-pack-in-win-win")) {
    return;
  }
  /* the type of the uiwidget is not known */
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  container = uiwidget->uidata.packwidget;
  widget = uiwidget->uidata.widget;

  if (uiwindow->wtype == WCONT_T_SCROLL_WINDOW) {
    NSScrollView  *scv;

    scv = uiwindow->uidata.widget;
    [scv setDocumentView : container];
    winbox = uiwindow->uidata.packwidget;
  }
  if (uiwindow->wtype == WCONT_T_WINDOW) {
    NSWindow    *win;

    win = uiwindow->uidata.widget;
    winbox = [win contentView];
    [winbox addView : container inGravity : grav];

    [winbox.safeAreaLayoutGuide.leadingAnchor
        constraintEqualToAnchor : container.leadingAnchor].active = YES;
    [winbox.safeAreaLayoutGuide.trailingAnchor
        constraintEqualToAnchor : container.trailingAnchor].active = YES;
    [winbox.safeAreaLayoutGuide.topAnchor
        constraintEqualToAnchor : container.topAnchor].active = YES;
    [winbox.safeAreaLayoutGuide.bottomAnchor
        constraintEqualToAnchor : container.bottomAnchor].active = YES;
  }

  uiwidget->packed = true;
  layout = uiwidget->uidata.layout;
  layout->expandchildren = true;

  winbox.autoresizingMask |= NSViewWidthSizable | NSViewHeightSizable;
  widget.autoresizingMask |= NSViewWidthSizable | NSViewHeightSizable;

  return;
}

void
uiWindowClearFocus (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "window-clear-focus")) {
    return;
  }

  return;
}

void
uiWindowGetMonitorSize (uiwcont_t *uiwindow, int *width, int *height)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-get-mon-sz")) {
    return;
  }

  *width = 0;
  *height = 0;
  return;
}
