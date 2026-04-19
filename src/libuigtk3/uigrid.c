/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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

#include "callback.h"
#include "oslocale.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uigrid.h"
#include "ui/uiui.h"

uiwcont_t *
uiCreateGrid (void)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *grid;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), uiBaseMarginSz * 2);
  uiwidget = uiwcontAlloc (WCONT_T_GRID, WCONT_T_GRID);
  uiwcontSetWidget (uiwidget, grid, NULL);

  return uiwidget;
}

#if 0
void
uiGridPostProcess_r (uiwcont_t *uibox, uiwcont_t *prev,
    uiwcont_t *curr, uiwcont_t *next)
{
  return;
}
#endif

void
uiGridPack (uiwcont_t *uigrid, uiwcont_t *uiwidget, int row, int col)
{
  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  uiGridPackSize (uigrid, uiwidget, row, col, 1, 1);
}

void
uiGridPackSize (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    int row, int col, int numrows, int numcols)
{
  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-size")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  gtk_grid_attach (GTK_GRID (uigrid->uidata.widget),
      uiwidget->uidata.packwidget, col, row, numcols, numrows);
}
