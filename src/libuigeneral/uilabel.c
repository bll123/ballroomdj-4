/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "uigeneral.h"
#include "uiwcont.h"

#include "ui/uilabel.h"

uiwcont_t *
uiCreateColonLabel (const char *txt)
{
  char      tbuff [400];

  if (txt == NULL) {
    return NULL;
  }

  snprintf (tbuff, sizeof (tbuff), "%s:", txt);
  return uiCreateLabel (tbuff);
}

