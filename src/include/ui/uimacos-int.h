/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
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

@interface IBox : NSStackView {}
- (BOOL) isFlipped;
@end

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

@interface IButton : NSButton {
  uiwcont_t   *uiwidget;
}
- (void) setUIWidget: (uiwcont_t *) tuiwidget;
- (IBAction) OnButton1Click: (id) sender;
- (BOOL) isFlipped;
@end

@interface IDWindow : NSWindow { }
@property uiwcont_t *uibox;
- (instancetype) init;
- (void) awakeFromNib;
@end

/* structures */

typedef struct macoslayout {
  NSEdgeInsets  margins;
  NSStackView   *container;
  bool          centered;
  bool          expand;
  bool          alignright;
} macoslayout_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

