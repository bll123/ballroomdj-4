/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiinternal.h"

uiwcont_t *
uiwcontAlloc (void)
{
  uiwcont_t    *uiwidget;

  uiwidget = mdmalloc (sizeof (uiwcont_t));
  uiwidget->widget = NULL;
  return uiwidget;
}

void
uiwcontFree (uiwcont_t *uiwidget)
{
  if (uiwidget != NULL) {
    mdfree (uiwidget);
  }
}

/* the following routines will be removed at a later date */

void
uiwcontInit (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }
  uiwidget->widget = NULL;
}

bool
uiwcontIsSet (uiwcont_t *uiwidget)
{
  bool rc = true;

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    rc = false;
  }
  return rc;
}


void
uiwcontCopy (uiwcont_t *target, uiwcont_t *source)
{
  if (target == NULL) {
    return;
  }
  if (source == NULL) {
    return;
  }
  memcpy (target, source, sizeof (uiwcont_t));
}

