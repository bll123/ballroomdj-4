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
  if (strcmp (tbuff, "ar") == 0 ||
      strcmp (tbuff, "dv") == 0 ||
      strcmp (tbuff, "fa") == 0 ||
      strcmp (tbuff, "ha") == 0 ||
      strcmp (tbuff, "he") == 0 ||
      strcmp (tbuff, "iw") == 0 ||
      strcmp (tbuff, "ji") == 0 ||
      strcmp (tbuff, "ps") == 0 ||
      strcmp (tbuff, "sd") == 0 ||
      strcmp (tbuff, "ug") == 0 ||
      strcmp (tbuff, "ur") == 0 ||
      strcmp (tbuff, "yi") == 0) {
    tdir = TEXT_DIR_RTL;
  }

  return tdir;
}
