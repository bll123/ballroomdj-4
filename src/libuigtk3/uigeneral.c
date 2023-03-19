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

#include "ui/uiinternal.h"
#include "ui/uigeneral.h"

uiwidget_t *
uiwidgetAlloc (void)
{
  uiwidget_t    *uiwidget;

  uiwidget = mdmalloc (sizeof (uiwidget_t));
  uiwidget->widget = NULL;
  return uiwidget;
}

void
uiwidgetFree (uiwidget_t *uiwidget)
{
  if (uiwidget != NULL) {
    mdfree (uiwidget);
  }
}

void
uiutilsUIWidgetInit (uiwidget_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }
  uiwidget->widget = NULL;
}

bool
uiutilsUIWidgetSet (uiwidget_t *uiwidget)
{
  bool rc = true;

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    rc = false;
  }
  return rc;
}


void
uiutilsUIWidgetCopy (uiwidget_t *target, uiwidget_t *source)
{
  if (target == NULL) {
    return;
  }
  if (source == NULL) {
    return;
  }
  memcpy (target, source, sizeof (uiwidget_t));
}

