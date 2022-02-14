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

#include "osuiutils.h"

void
osuiSetIcon (char *fname)
{
  NSImage *image = nil;

  NSString *ns = [NSString stringWithUTF8String: fname];
  image = [[NSImage alloc] initWithContentsOfFile: ns];
  [[NSApplication sharedApplication] setApplicationIconImage:image];
  [image release];
}

void
osuiSetWindowAsAccessory (void)
{
  [[NSApplication sharedApplication]
      setActivationPolicy: NSApplicationActivationPolicyAccessory];
}

#endif /* __APPLE__ */
