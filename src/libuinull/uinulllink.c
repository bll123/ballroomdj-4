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
#include <math.h>

#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uilink.h"

uiwcont_t *
uiCreateLink (const char *label, const char *uri)
{
  return NULL;
}

void
uiLinkSet (uiwcont_t *uilink, const char *label, const char *uri)
{
  return;
}

void
uiLinkSetActivateCallback (uiwcont_t *uilink, callback_t *uicb)
{
  return;
}
