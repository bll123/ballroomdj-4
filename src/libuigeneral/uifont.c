/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

/* this file is currently not in use 2024-12-2 */

/* if this doesn't see use, remove it later */
void
uiFontInfo (const char *font, char *buff, size_t sz, int *fontsz) /* UNUSED */
{
  size_t      i;
  ssize_t     idx = -1;

  stpecpy (buff, buff + sz, font);

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

