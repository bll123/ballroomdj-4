/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "bdj4itunes.h"
#include "bdjopt.h"
#include "fileop.h"

bool
itunesConfigured (void)
{
  const char  *tval;
  int         have = 0;

  tval = bdjoptGetStr (OPT_M_DIR_ITUNES_MEDIA);
  if (fileopIsDirectory (tval)) {
    ++have;
  }
  tval = bdjoptGetStr (OPT_M_ITUNES_XML_FILE);
  if (fileopFileExists (tval)) {
    ++have;
  }
  return have == 2;
}
