/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if __APPLE__

#import <Foundation/NSObject.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <MacTypes.h>
#include <Cocoa/Cocoa.h>

#include "osuiutils.h"

void
osuiSetIcon (const char *fname)
{
  NSImage   *image = nil;
  NSString  *ns = nil;

  ns = [NSString stringWithUTF8String: fname];
  image = [[NSImage alloc] initWithContentsOfFile: ns];
  [[NSApplication sharedApplication] setApplicationIconImage:image];
  [image release];
  [ns release];
}

void
osuiFinalize (void)
{
  [NSApp finishLaunching];
  return;
}

#endif /* __APPLE__ */
