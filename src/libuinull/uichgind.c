/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uichgind.h"
#include "ui/uigrid.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

uiwcont_t *
uiCreateChangeIndicator (void)
{
  return NULL;
}

void
uichgindPackGrid (uiwcont_t *uiwidget, uiwcont_t *grid)
{
  if (! uiwcontValid (uiwidget, WCONT_T_CHGIND, "ci-pack-grid")) {
    return;
  }
  if (! uiwcontValid (grid, WCONT_T_GRID, "ci-pack-grid")) {
    return;
  }

  uiGridPack (grid, uiwidget, WCONT_KEEP);
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
