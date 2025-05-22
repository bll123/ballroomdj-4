/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bdj4intl.h"
#include "uigeneral.h"
#include "uiwcont.h"

#include "ui/uilabel.h"

uiwcont_t *
uiCreateColonLabel (const char *txt)
{
  char        tbuff [600];
  const char  *colonstr;

  if (txt == NULL) {
    return NULL;
  }

  /* CONTEXT: colon character */
  colonstr = _(":");
  snprintf (tbuff, sizeof (tbuff), "%s%s", txt, colonstr);
  return uiCreateLabel (tbuff);
}

