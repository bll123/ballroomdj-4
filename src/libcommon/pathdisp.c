/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "mdebug.h"
#include "pathdisp.h"
#include "sysvars.h"

void
pathDisplayPath (char *buff, size_t len)
{
  if (! isWindows ()) {
    return;
  }

  for (size_t i = 0; i < len; ++i) {
    if (buff [i] == '\0') {
      break;
    }
    if (buff [i] == '/') {
      buff [i] = '\\';
    }
  }
}
