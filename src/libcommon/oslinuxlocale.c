/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if __linux__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "oslocale.h"

int
osLocaleDirection (const char *locale)
{
  int       tdir = TEXT_DIR_LTR;

  return tdir;
}

#endif
