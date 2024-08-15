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

#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uibox.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

@interface IWindowDelegate : NSObject
- (void)windowDidBecomeKey:(NSNotification *)notification;
- (void)windowDidBecomeMain:(NSNotification *)notification;
- (void)windowDidResignKey:(NSNotification *)notification;
- (void)windowDidResignMain:(NSNotification *)notification;
- (void)windowWillClose:(NSNotification *)notification;
- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
- (IBAction) OnButton1Click:(id)sender;
- (IBAction) OnButton2Click:(id)sender;
@end

@interface IWindow : NSWindow { }
@property uiwcont_t *uibox;
- (instancetype) init;
- (void) awakeFromNib;
@end

@implementation IWindow

- (instancetype)init {

  [super initWithContentRect:NSMakeRect(10, 10, 100, 100)
      styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
          NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
      backing:NSBackingStoreBuffered
      defer:NO];
  [self setIsVisible:YES];
  return self;
}

- (void)awakeFromNib {
  IWindow*  w = self;
  NSRect    f;
  NSStackView *box;
  NSView    *clg;
  NSSize    nssz;

  box = [w contentView];
// ### this doesn't seem to work.
// but leave it here just in case.
  nssz = [box fittingSize];
  f.size.height = nssz.height;
  f.size.width  = nssz.width;
  [w setFrame:f display:YES];

// ### this doesn't seem to be working either
  clg = w.contentLayoutGuide;
//  [clg.leadingAnchor constraintEqualToAnchor: box.leadingAnchor].active = YES;
//  [clg.trailingAnchor constraintEqualToAnchor: box.trailingAnchor].active = YES;
//  [clg.topAnchor constraintEqualToAnchor: box.topAnchor].active = YES;
//  [clg.bottomAnchor constraintEqualToAnchor: box.bottomAnchor].active = YES;
  [clg.heightAnchor constraintEqualToAnchor: box.heightAnchor].active = YES;
  [clg.widthAnchor constraintEqualToAnchor: box.widthAnchor].active = YES;
}

@end

@implementation IWindowDelegate : NSObject

- (void)windowDidBecomeKey:(NSNotification *)notification {
    NSLog(@"Window: become key");
}

- (void)windowDidBecomeMain:(NSNotification *)notification {
    NSLog(@"Window: become main");
}

- (void)windowDidResignKey:(NSNotification *)notification {
    NSLog(@"Window: resign key");
}

- (void)windowDidResignMain:(NSNotification *)notification {
    NSLog(@"Window: resign main");
}

- (BOOL)canBecomeKeyWindow {
    // Required for NSWindowStyleMaskBorderless windows
    return YES;
}

- (BOOL)canBecomeMainWindow {
    return YES;
}

- (IBAction) OnButton1Click:(id)sender {
NSLog(@"button-1");
}

- (IBAction) OnButton2Click:(id)sender {
NSLog(@"button-2");
}

// This will close/terminate the application when the main window is closed.
- (void)windowWillClose:(NSNotification *)notification {
    IWindow *window = notification.object;
NSLog(@"Window: closing");
    if (window.isMainWindow) {
      [NSApp terminate:nil];
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

    nstitle = [NSString stringWithUTF8String: title];
    [win setTitle: nstitle];
  }

  box = uibox->uidata.widget;
  [win setContentView: box];
  [win makeMainWindow];

  uibox->packed = true;

  if (imagenm != NULL) {
    NSImage *image = nil;

    NSString *ns = [NSString stringWithUTF8String: imagenm];
    image = [[NSImage alloc] initWithContentsOfFile: ns];
    [[NSApplication sharedApplication] setApplicationIconImage:image];
    [image release];
  }

  windowDelegate = [[IWindowDelegate alloc] init];
  [win setDelegate:windowDelegate];

  uiwin = uiwcontAlloc ();
  uiwin->wbasetype = WCONT_T_WINDOW;
  uiwin->wtype = WCONT_T_WINDOW;
  uiwin->uidata.widget = win;
  uiwin->uidata.packwidget = win;
  uiwin->packed = true;

  uiWidgetSetAllMargins (uibox, 2);
  uiWidgetExpandHoriz (uibox);
  uiWidgetExpandVert (uibox);

  return uiwin;
}

void
uiWindowSetTitle (uiwcont_t *uiwindow, const char *title)
{
  IWindow   *win = NULL;
  NSString  *nstitle = [NSString stringWithUTF8String: title];

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-title")) {
    return;
  }

  win = uiwindow->uidata.widget;
  [win setTitle:nstitle];
  return;
}

void
uiCloseWindow (uiwcont_t *uiwindow)
{
  return;
}

bool
uiWindowIsMaximized (uiwcont_t *uiwindow)
{
  return false;
}

void
uiWindowIconify (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowDeIconify (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowMaximize (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowUnMaximize (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowDisableDecorations (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowEnableDecorations (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowGetSize (uiwcont_t *uiwindow, int *x, int *y)
{
  return;
}

void
uiWindowSetDefaultSize (uiwcont_t *uiwindow, int x, int y)
{
  return;
}

void
uiWindowGetPosition (uiwcont_t *uiwindow, int *x, int *y, int *ws)
{
  return;
}

void
uiWindowMove (uiwcont_t *uiwindow, int x, int y, int ws)
{
  return;
}

void
uiWindowMoveToCurrentWorkspace (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowNoFocusOnStartup (uiwcont_t *uiwindow)
{
  return;
}

uiwcont_t *
uiCreateScrolledWindow (int minheight)
{
  return NULL;
}

void
uiWindowSetPolicyExternal (uiwcont_t *uisw)
{
  return;
}

uiwcont_t *
uiCreateDialogWindow (uiwcont_t *parentwin,
    uiwcont_t *attachment, callback_t *uicb, const char *title)
{
  return NULL;
}

void
uiWindowSetDoubleClickCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

void
uiWindowSetWinStateCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  return;
}

void
uiWindowNoDim (uiwcont_t *uiwidget)
{
  return;
}

void
uiWindowSetMappedCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

void
uiWindowPresent (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowRaise (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowFind (uiwcont_t *window)
{
  return;
}

void
uiWindowSetNoMaximize (uiwcont_t *uiwindow)
{
  int      sm;
  IWindow  *win;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-nomax")) {
    return;
  }

  win = uiwindow->uidata.widget;
  sm = win.styleMask;
  sm &= ~NSWindowStyleMaskResizable;
  win.styleMask = sm;
  return;
}

void
uiWindowPackInWindow (uiwcont_t *uiwindow, uiwcont_t *uiwidget)
{
  NSWindow    *win;
  NSView      *widget = NULL;
  NSStackView *box;
  int         grav = NSStackViewGravityTop;

  if (uiwindow == NULL || uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  win = uiwindow->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  box = [win contentView];
  [box addView: widget inGravity: grav];
  uiwidget->packed = true;
  return;
}

void
uiWindowClearFocus (uiwcont_t *uiwidget)
{
  return;
}
