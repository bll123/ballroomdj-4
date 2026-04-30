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

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uimacos-int.h"
#include "ui/uiwcont-int.h"

#include "ui/uigrid.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

@implementation IGrid
- (BOOL) isFlipped {
  return YES;
}
@end

static void uiGridSetHorizAlignment (NSGridView *grid, int row, int col, int alignment);
static void uiGridSetVertAlignment (NSGridView *grid, int row, int col, int alignment);

uiwcont_t *
uiCreateGrid (void)
{
  uiwcont_t   *uiwidget;
  IGrid       *grid;
  uigridint_t *uigridint;

  uigridint = mdmalloc (sizeof (uigridint_t));
  uigridint->row = 0;
  uigridint->col = 0;
  uigridint->new = true;

  grid = [[IGrid alloc] init];
  grid.columnSpacing = uiBaseMarginSz * 2;

  uiwidget = uiwcontAlloc (WCONT_T_GRID, WCONT_T_GRID);
  uiwcontSetWidget (uiwidget, grid, NULL);
  uiwidget->uiint.uigrid = uigridint;

  uiWidgetSetIdentifier (uiwidget, "grid");

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
  uigridint_t    *uigridint;

  if (! uiwcontValid (uiwidget, WCONT_T_GRID, "grid-free")) {
    return;
  }

  uigridint = uiwidget->uiint.uigrid;
  mdfree (uigridint);
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
uiGridPackFill (uiwcont_t *uigrid, uiwcont_t *uiwidget)
{
  macoslayout_t   *layout;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->horizfill = true;

  /* pack-size will create the cell, do this first */
  uiGridPackSize (uigrid, uiwidget, 1, 1);
}

void
uiGridPackHorizCenter (uiwcont_t *uigrid, uiwcont_t *uiwidget)
{
  macoslayout_t   *layout;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->horizcenter = true;

  /* pack-size will create the cell, do this first */
  uiGridPackSize (uigrid, uiwidget, 1, 1);
}

void
uiGridPackSize (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    int numrows, int numcols)
{
  uigridint_t   *uigridint;
  IGrid         *grid;
  macoslayout_t *layout;
  int           alignment;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-size")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  uigridint = uigrid->uiint.uigrid;
  grid = uigrid->uidata.widget;
  layout = uiwidget->uidata.layout;

fprintf (stderr, "grid: %d-%d p: %d-%d %d/%s\n", uigrid->ui_id, uigrid->id, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
fprintf (stderr, "   col: %d row: %d\n", uigridint->col, uigridint->row);
fprintf (stderr, "   cols: %ld rows: %ld\n", grid.numberOfColumns, grid.numberOfRows);
  if (uigridint->col == 0) {
    [grid addRowWithViews : @[layout->container]];
  } else {
    [grid addColumnWithViews : @[layout->container]];
  }
  if (numcols > 1 || numrows > 1) {
    [grid
        mergeCellsInHorizontalRange : NSMakeRange (uigridint->row, numcols)
        verticalRange: NSMakeRange (uigridint->col, numrows)];
  }

fprintf (stderr, "   row: %d col: %d leading\n", uigridint->row, uigridint->col);
  alignment = NSGridCellPlacementLeading;
  layout = uiwidget->uidata.layout;
  if (layout->horizend) {
    alignment = NSGridCellPlacementTrailing;
  }
  if (layout->expandhoriz || layout->horizfill) {
    alignment = NSGridCellPlacementFill;
  }
  if (layout->horizcenter) {
    alignment = NSGridCellPlacementCenter;
  }

  uiGridSetHorizAlignment (grid, uigridint->row, uigridint->col, alignment);

  alignment = NSGridCellPlacementCenter;
  layout = uiwidget->uidata.layout;
  if (layout->vertend) {
    alignment = NSGridCellPlacementTrailing;
  }
  if (layout->expandvert || layout->vertfill) {
    alignment = NSGridCellPlacementFill;
  }
  if (layout->vertstart) {
    alignment = NSGridCellPlacementLeading;
  }

  uiGridSetVertAlignment (grid, uigridint->row, uigridint->col, alignment);

  /* numrows is not handled properly */
  uigridint->col += numcols;
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

static void
uiGridSetHorizAlignment (NSGridView *grid, int row, int col, int alignment)
{
  NSGridColumn  *gcol;
  NSGridCell    *gcell;

  gcol = [grid columnAtIndex : col];
  if (gcol == nil) {
    fprintf (stderr, "ERR: no column at %d\n", col);
    return;
  }

  gcell = [gcol cellAtIndex : row];
  if (gcell == nil) {
    fprintf (stderr, "ERR: no cell at %d/%d\n", row, col);
    return;
  }

  gcell.xPlacement = alignment;
}

static void
uiGridSetVertAlignment (NSGridView *grid, int row, int col, int alignment)
{
  NSGridColumn  *gcol;
  NSGridCell    *gcell;

  gcol = [grid columnAtIndex : col];
  if (gcol == nil) {
    fprintf (stderr, "ERR: no column at %d\n", col);
    return;
  }

  gcell = [gcol cellAtIndex : row];
  if (gcell == nil) {
    fprintf (stderr, "ERR: no cell at %d/%d\n", row, col);
    return;
  }

  gcell.yPlacement = alignment;
}

