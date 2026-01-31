/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <Cocoa/Cocoa.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* interfaces */

@interface ILabel : NSTextField {}
- (BOOL) isFlipped;
@end

@interface IWindowDelegate : NSObject { }
- (void) windowDidBecomeKey : (NSNotification *) notification;
- (void) windowDidBecomeMain : (NSNotification *) notification;
- (void) windowDidResignKey : (NSNotification *) notification;
- (void) windowDidResignMain : (NSNotification *) notification;
- (void) windowWillClose : (NSNotification *) notification;
- (BOOL) canBecomeKeyWindow;
- (BOOL) canBecomeMainWindow;
- (IBAction) OnButton2Click : (id) sender;
@end

@interface IButton : NSButton {
  uiwcont_t   *uiwidget;
  callback_t  *cb;
}
- (void) setUIWidget : (uiwcont_t *) tuiwidget;
- (void) setCallback : (callback_t *) tcb;
- (IBAction) OnButton1Click : (id) sender;
- (BOOL) isFlipped;
@end

/* additional routines */

void uiWidgetUpdateLayout (uiwcont_t *uiwidget);

/* structures */

typedef struct macoslayout {
  NSStackView   *container;
  NSEdgeInsets  margins;
  bool          expandchildren;
} macoslayout_t;

#define MACOS_UI_DEBUG 1

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

