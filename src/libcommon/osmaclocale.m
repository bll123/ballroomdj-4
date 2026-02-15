/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#import <Foundation/NSObject.h>
#import <Foundation/NSLocale.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <MacTypes.h>
#include <Cocoa/Cocoa.h>

#include "bdjstring.h"
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

void
osGetPreferredLocales (char *buff, size_t sz)
{
  *buff = '\0';

  /* though the list of languages is fetched properly, the libintl */
  /* library does not process the LANGUAGE environment variable */
  /* correctly */
#if 0
  NSArray   *langs;
  int       lcount;
  char      *p;

  *buff = '\0';
  p = buff;

  langs = [NSLocale preferredLanguages];
  lcount = [langs count];
  for (int i = 0; i < lcount; ++i) {
    NSString    *lang;
    char        tbuff [20];

    lang = [langs objectAtIndex: i];
    stpecpy (tbuff, tbuff + sizeof (tbuff), [lang UTF8String]);
    /* the specific language first */
    tbuff [2] = '_';
    p = stpecpy (p, buff + sz, tbuff);

    /* the general language of the specific */
    tbuff [2] = '\0';
    p = stpecpy (p, buff + sz, ":");
    p = stpecpy (p, buff + sz, tbuff);

    if (i < lcount - 1) {
      p = stpecpy (p, buff + sz, ":");
    }
  }
#endif
}
