/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bdjstring.h"
#include "uigeneral.h"

void
uiFontInfo (const char *font, char *buff, size_t sz, int *fontsz)
{
  size_t      i;
  ssize_t     idx = -1;

  strlcpy (buff, font, sz);

  i = strlen (buff) - 1;
  while (i != 0 && (isdigit (buff [i]) || isspace (buff [i]))) {
    if (idx == -1) {
      idx = i;
    }
    --i;
  }
  ++i;
  buff [i] = '\0';
  *fontsz = atoi (&buff [idx]);
}

