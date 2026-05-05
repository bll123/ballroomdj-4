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
uiCreateGrid_r (void)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *grid;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), uiBaseMarginSz * 2);
  uiwidget = uiwcontAlloc (WCONT_T_GRID, WCONT_T_GRID);
  uiwcontSetWidget (uiwidget, grid, NULL);

  return uiwidget;
}

void
uiGridPackLoc_r (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    int row, int col, int numcols)
{
  uigridgeneric_t    *uigridgen;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-size")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->packwidget == NULL) {
    return;
  }

  uigridgen = uigrid->uigridgen;
  gtk_grid_attach (GTK_GRID (uigrid->widget),
      uiwidget->packwidget, col, row, numcols, 1);
}

void
uiGridPostProcess_r (uiwcont_t *uigrid, int row, int col, uiwcont_t *uiwidget)
{
  return;
}

void
uiGridSetColumnSpacing (uiwcont_t *uigrid, int sz)
{
  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-rm-row")) {
    return;
  }

  gtk_grid_set_column_spacing (GTK_GRID (uigrid->widget),
      uiBaseMarginSz * sz);
}
