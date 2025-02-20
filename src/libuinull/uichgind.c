/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uibox.h"
#include "ui/uiwidget.h"
#include "ui/uichgind.h"


uiwcont_t *
uiCreateChangeIndicator (uiwcont_t *boxp)
{
  return NULL;
}

void
uichgindMarkNormal (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-mark-norm")) {
    return;
  }

  return;
}

void
uichgindMarkChanged (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-mark-chg")) {
    return;
  }

  return;
}
