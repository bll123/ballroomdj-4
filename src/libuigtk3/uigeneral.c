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

#include "ui/uiinternal.h"
#include "ui/uigeneral.h"

void
uiutilsUIWidgetInit (UIWidget *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }
  uiwidget->widget = NULL;
}

bool
uiutilsUIWidgetSet (UIWidget *uiwidget)
{
  bool rc = true;

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    rc = false;
  }
  return rc;
}


void
uiutilsUIWidgetCopy (UIWidget *target, UIWidget *source)
{
  if (target == NULL) {
    return;
  }
  if (source == NULL) {
    return;
  }
  memcpy (target, source, sizeof (UIWidget));
}

