/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if __APPLE__

#import "Foundation/NSObject.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <MacTypes.h>
#include <Cocoa/Cocoa.h>

#include "ui/uios.h"

void
uiosWindowPresent (UIWidget *uiwindow)
{
  GdkWindow *gdkwin;
  NSWindow *nswin = nil;
  NSWindow *gdk_quartz_window_get_nswindow (void *);

  gdkwin = gtk_widget_get_window (uiwindow->widget);
  nswin = gdk_quartz_window_get_nswindow (gdkwin);
fprintf (stderr, "gdkwin: %p nswin: %p\n", gdkwin, nswin);
  if (nswin != nil) {
    int valk = [nswin canBecomeKeyWindow];
fprintf (stderr, "can-become-key: %d\n", valk);
    int valm = [nswin canBecomeMainWindow];
fprintf (stderr, "can-become-main: %d\n", valm);
    [nswin makeKeyAndOrderFront: nswin];
  }
}

#endif /* __APPLE__ */
