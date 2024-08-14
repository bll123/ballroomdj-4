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

@interface IWindow : NSWindow {}
- (instancetype)init;
@end

@implementation IWindow
- (instancetype)init {

  [super initWithContentRect:NSMakeRect(100, 100, 300, 300)
      styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
          NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
      backing:NSBackingStoreBuffered
      defer:NO];
  [self setIsVisible:YES];
  return self;
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
  uiwcont_t *uiwin;
  IWindow   *win = NULL;
  uiwcont_t *uibox;
  id        windowDelegate;

  win = [[IWindow alloc] init];
  uibox = uiCreateVertBox ();
  if (title != NULL) {
    NSString  *nstitle;

    nstitle = [NSString stringWithUTF8String: title];
    [win setTitle: nstitle];
  }
  [win setContentView: uibox->uidata.widget];
  [win makeMainWindow];

  if (imagenm != NULL) {
    NSImage *image = nil;

    NSString *ns = [NSString stringWithUTF8String: imagenm];
    image = [[NSImage alloc] initWithContentsOfFile: ns];
    [[NSApplication sharedApplication] setApplicationIconImage:image];
    [image release];
  }

  windowDelegate = [[IWindowDelegate alloc] init];
  [win setDelegate:windowDelegate];
{
NSTextField *l;
  int       grav = NSStackViewGravityTop;

l = [[NSTextField alloc] init];
[l setBezeled:NO];
[l setDrawsBackground:NO];
[l setEditable:NO];
[l setStringValue: [NSString stringWithUTF8String: title]];
[[win contentView] addView: l inGravity:grav];
}

  uiwin = uiwcontAlloc ();
  uiwin->wbasetype = WCONT_T_WINDOW;
  uiwin->wtype = WCONT_T_WINDOW;
  uiwin->uidata.widget = win;
  uiwin->uidata.packwidget = win;
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
  return;
}

void
uiWindowPackInWindow (uiwcont_t *uiwindow, uiwcont_t *uiwidget)
{
  IWindow   *win;
  NSView    *widget = NULL;
  int       grav = NSStackViewGravityTop;

  if (uiwindow == NULL || uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  win = uiwindow->uidata.widget;
  widget = uiwidget->uidata.packwidget;
{
NSTextField *l;

l = [[NSTextField alloc] init];
[l setBezeled:NO];
[l setDrawsBackground:NO];
[l setEditable:NO];
[l setStringValue: @"win"];
[[win contentView] addView: l inGravity:grav];
}
  [[win contentView] addView: widget inGravity:grav];
  return;
}

void
uiWindowClearFocus (uiwcont_t *uiwidget)
{
  return;
}
