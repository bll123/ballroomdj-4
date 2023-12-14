/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if __APPLE__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#import <Foundation/NSLocale.h>

#include "oslocale.h"

static char             *reasonstr = NULL;

int
osLocaleDirection (const char *locale)
{
  int       tdir = TEXT_DIR_LTR;
  int       mtdir = NSLocaleLanguageDirectionLeftToRight;

  mtdir = [NSLocale characterDirectionForLanguage:
      [[NSLocale currentLocale] objectForKey:NSLocaleLanguageCode]];
  if (mtdir == NSLocaleLanguageDirectionRightToLeft) {
    tdir = TEXT_DIR_RTL;
  }

  return tdir;
}

#endif
