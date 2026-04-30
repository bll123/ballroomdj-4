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

#include <gtk/gtk.h>

#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uigrid.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

uiwcont_t *
uiCreateGrid (void)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *grid;
  uigridint_t    *uigrid;

  uigrid = mdmalloc (sizeof (uigridint_t));
  uigrid->row = 0;
  uigrid->col = 0;
  uigrid->new = true;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), uiBaseMarginSz * 2);
  uiwidget = uiwcontAlloc (WCONT_T_GRID, WCONT_T_GRID);
  uiwcontSetWidget (uiwidget, grid, NULL);
  uiwidget->uiint.uigrid = uigrid;

  return uiwidget;
}

#if 0
void
uiGridPostProcess_r (uiwcont_t *uigrid, uiwcont_t *prev,
    uiwcont_t *curr, uiwcont_t *next)
{
  return;
}
#endif

void
uiGridFree (uiwcont_t *uiwidget)
{
  uigridint_t    *uigrid;

  if (! uiwcontValid (uiwidget, WCONT_T_GRID, "grid-free")) {
    return;
  }

  uigrid = uiwidget->uiint.uigrid;
  mdfree (uigrid);
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

  uiGridPackSize (uigrid, uiwidget, 1, 1);
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
  uigridint_t    *uigridint;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-size")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  uigridint = uigrid->uiint.uigrid;
  gtk_grid_attach (GTK_GRID (uigrid->uidata.widget),
      uiwidget->uidata.packwidget, uigridint->col, uigridint->row,
      numcols, numrows);
  uigridint->col += 1;
  uigridint = uigrid->uiint.uigrid;
  uigridint->new = false;
}

void
uiGridNewRow (uiwcont_t *uiwidget)
{
  uigridint_t    *uigrid;

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
