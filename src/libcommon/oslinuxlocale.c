/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "oslocale.h"

int
osLocaleDirection (const char *locale)
{
  int       tdir = TEXT_DIR_LTR;
  char      tbuff [3];

  strncpy (tbuff, locale, 2);
  tbuff [2] = '\0';

  /* I don't know that there is any better way for Linux */
  if (strcmp (tbuff, "ar") == 0 ||      // arabic
      strcmp (tbuff, "dv") == 0 ||      // divehi
      strcmp (tbuff, "fa") == 0 ||      // persian
      strcmp (tbuff, "ff") == 0 ||      // fulah
      strcmp (tbuff, "ha") == 0 ||      // hausa
      strcmp (tbuff, "he") == 0 ||      // hebrew
      strcmp (tbuff, "iw") == 0 ||      // old hebrew
      strcmp (tbuff, "ks") == 0 ||      // kashmiri
      strcmp (tbuff, "ku") == 0 ||      // kurdish
      strcmp (tbuff, "ps") == 0 ||      // pashto, pushto
      strcmp (tbuff, "sd") == 0 ||      // sindhi
      strcmp (tbuff, "ug") == 0 ||      // uighur
      strcmp (tbuff, "ur") == 0 ||      // urdu
      strcmp (tbuff, "yi") == 0) {      // yezidi
    tdir = TEXT_DIR_RTL;
  }

  return tdir;
}
