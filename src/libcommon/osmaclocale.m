/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if __APPLE__

#import <Foundation/NSObject.h>
#import <Foundation/NSLocale.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <MacTypes.h>
#include <Cocoa/Cocoa.h>

#include "oslocale.h"

int
osLocaleDirection (const char *locale)
{
  int       tdir = TEXT_DIR_LTR;
  int       mtdir = NSLocaleLanguageDirectionLeftToRight;
  NSString  *nslocale = NULL;
  NSLocale  *mlocale = NULL;

  nslocale = [NSString stringWithUTF8String: locale];
  mlocale = [[[NSLocale alloc] initWithLocaleIdentifier:nslocale] autorelease];
  mtdir = [NSLocale characterDirectionForLanguage:
      [mlocale objectForKey:NSLocaleLanguageCode]];
  if (mtdir == NSLocaleLanguageDirectionRightToLeft) {
    tdir = TEXT_DIR_RTL;
  }

  return tdir;
}

#endif
