/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
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
//  IWindow*  w = self;
//  NSStackView *box;

//  box = [w contentView];
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

fprintf (stderr, "c-main-win\n");
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

  uiwin = uiwcontAlloc (WCONT_T_WINDOW, WCONT_T_WINDOW);
  uiwcontSetWidget (uiwin, win, NULL);
  uiwin->packed = true;

  uiWidgetSetAllMargins (uibox, 2);
// ### these should not be necessary
//  uiWidgetExpandHoriz (uibox);
//  uiWidgetExpandVert (uibox);

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

  nstitle = [NSString stringWithUTF8String: title];

  win = uiwindow->uidata.widget;
  [win setTitle:nstitle];
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
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-dflt-sz")) {
    return;
  }

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
fprintf (stderr, "c-scroll-win\n");
  return NULL;
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

fprintf (stderr, "c-dialog-win\n");
  return NULL;
}

void
uiWindowSetDoubleClickCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-dclick-cb")) {
    return;
  }

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
  NSWindow      *win;
  NSStackView   *widget = NULL;
  NSStackView   *winbox;
  int           grav = NSStackViewGravityTop;
  macoslayout_t *layout = NULL;

  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-pack-in-win-win")) {
    return;
  }
  /* the type of the uiwidget is not known */
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  win = uiwindow->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  winbox = [win contentView];
  [winbox addView: widget inGravity: grav];
  layout = uiwidget->uidata.layout;

fprintf (stderr, "  add pack-in-win constraint\n");
  [widget.leadingAnchor
      constraintEqualToAnchor: winbox.leadingAnchor
      constant: layout->margins.left].active = YES;
  [widget.trailingAnchor
      constraintEqualToAnchor: winbox.trailingAnchor
      constant: layout->margins.right].active = YES;
  [widget.topAnchor
      constraintEqualToAnchor: winbox.topAnchor
      constant: layout->margins.top].active = YES;
  [widget.bottomAnchor
      constraintEqualToAnchor: winbox.bottomAnchor
      constant: layout->margins.bottom].active = YES;
  [widget setAutoresizingMask : NSViewWidthSizable | NSViewHeightSizable];
  layout->expand = true;

  uiwidget->packed = true;
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
