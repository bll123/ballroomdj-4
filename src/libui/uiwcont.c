/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

uiwcont_t *
uiwcontAlloc (void)
{
  uiwcont_t    *uiwidget;

  uiwidget = mdmalloc (sizeof (uiwcont_t));
  uiwidget->wtype = WCONT_T_UNKNOWN;
  uiwidget->widget = NULL;
  uiwidget->uiint.voidwidget = NULL;    // union
  return uiwidget;
}

void
uiwcontBaseFree (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }

  uiwidget->wtype = WCONT_T_UNKNOWN;
  mdfree (uiwidget);
}

