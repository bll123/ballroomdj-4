/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uigrid.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

uiwcont_t *
uiCreateGrid (void)
{
  return NULL;
}

void
uiGridFree (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack")) {
    return;
  }

  return;
}

void
uiGridPack (uiwcont_t *uigrid, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  return;
}

void
uiGridPackNoExpand (uiwcont_t *uigrid, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  uiWidgetAlignHorizStart (uiwidget);
  uiGridPackSize (uigrid, uiwidget, 1, 1);
}

void
uiGridPackSize (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    int numrows, int numcols)
{
  uigrid_t    *uigridint;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-size")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  uigridint = uigrid->uiint.uigrid;


  uigridint->col += 1;
  uigridint = uigrid->uiint.uigrid;
  uigridint->new = false;
  return;
}

void
uiGridNewRow (uiwcont_t *uiwidget)
{
  uigrid_t    *uigrid;

  if (! uiwcontValid (uiwidget, WCONT_T_GRID, "grid-pack-size")) {
    return;
  }

  uigrid = uiwidget->uiint.uigrid;
  if (uigrid->new) {
    return;
  }
  uigrid->row += 1;
  uigrid->col = 0;
}
