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

#define MACOS_UI_DEBUG 1

/* structures */

typedef struct macoslayout {
  NSStackView   *container;
  NSEdgeInsets  margins;
  bool          centered;
  bool          expand;
  bool          alignright;
} macoslayout_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

