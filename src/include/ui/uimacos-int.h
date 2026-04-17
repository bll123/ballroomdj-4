/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <Cocoa/Cocoa.h>

#include "callback.h"
#include "uiwcont.h"

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

void uiWidgetSetIdentifier (uiwcont_t *uiwidget, const char *tag);
void uiWidgetUpdateLayout (uiwcont_t *uiwidget);
void uiWidgetUpdateConstraint (const char *tag, NSLayoutConstraint *c, NSView *va, NSView *vb);

/* structures */

typedef struct macoslayout {
  NSLayoutGuide *sizegroup;
  NSStackView   *container;
  NSEdgeInsets  margins;
  bool          centerhoriz;
  bool          centervert;
  bool          expandhoriz;
  bool          expandvert;
  bool          alignright;
  bool          expandchildren;
} macoslayout_t;

#define MACOS_UI_DEBUG 1

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif
