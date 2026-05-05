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
uiCreateGrid_r (void)
{
  uiwcont_t   *uiwidget;
  IGrid       *grid;

  grid = [[IGrid alloc] init];
  grid.columnSpacing = uiBaseMarginSz * 2;
//  grid.rowSpacing = uiBaseMarginSz;
  grid.rowSpacing = 0;
  grid.rowAlignment = NSGridRowAlignmentFirstBaseline;

  uiwidget = uiwcontAlloc (WCONT_T_GRID, WCONT_T_GRID);
  uiwcontSetWidget (uiwidget, grid, NULL);

  uiWidgetSetIdentifier (uiwidget, "grid");

  return uiwidget;
}

/* this is a little more complex, as there is a need to both */
/* add an item by row, and add an item by column */
void
uiGridPackLoc_r (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    int row, int col, int numcols)
{
  uigridint_t   *uigridint;
  IGrid         *grid;
  macoslayout_t *layout;
  int           ncols, nrows;
  NSGridColumn  *gcol;
  NSGridCell    *gcell;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-size")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  uigridint = uigrid->uiint.uigridint;
  grid = uigrid->uidata.widget;
  layout = uiwidget->uidata.layout;

//fprintf (stderr, "grid: %d-%d p: %d-%d %d/%s\n", uigrid->ui_id, uigrid->id, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
//fprintf (stderr, "   col: %d row: %d\n", uigridint->col, uigridint->row);

  ncols = grid.numberOfColumns;
  nrows = grid.numberOfRows;

  while (row >= nrows) {
    [grid addRowWithViews : @[]];
    nrows = grid.numberOfRows;
  }
  while (col + numcols > ncols) {
    [grid addColumnWithViews : @[]];
    ncols = grid.numberOfColumns;
  }

  if (row >= grid.numberOfRows || col >= grid.numberOfColumns) {
    fprintf (stderr, "ERR: tried to set cell at %d/%d limit %ld/%ld\n", row, col, grid.numberOfRows, grid.numberOfColumns);
    return;
  }

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

  gcell.contentView = layout->container;

  if (numcols > 1) {
    [grid
        mergeCellsInHorizontalRange : NSMakeRange (uigridint->row, numcols)
        verticalRange: NSMakeRange (uigridint->col, 1)];
  }
}

void
uiGridPostProcess_r (uiwcont_t *uigrid, int row, int col, uiwcont_t *uiwidget)
{
  IGrid         *grid;
  macoslayout_t *layout;
  int           alignment;

//fprintf (stderr, "grid: post: %d-%d %d %d : %d-%d %d/%s\n", uigrid->ui_id, uigrid->id, row, col, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  grid = uigrid->uidata.widget;
  layout = uiwidget->uidata.layout;

  /* align-horiz-start is the default */
  alignment = NSGridCellPlacementLeading;
  if (layout->horizend) {
    alignment = NSGridCellPlacementTrailing;
  }
  if (layout->expandhoriz || layout->horizfill) {
    alignment = NSGridCellPlacementFill;
  }
  if (layout->horizcenter) {
    alignment = NSGridCellPlacementCenter;
  }

  uiGridSetHorizAlignment (grid, row, col, alignment);

  alignment = NSGridCellPlacementCenter;
  if (layout->vertend) {
    alignment = NSGridCellPlacementTrailing;
  }
  if (layout->expandvert || layout->vertfill) {
    alignment = NSGridCellPlacementFill;
  }
  if (layout->vertstart) {
    alignment = NSGridCellPlacementLeading;
  }

  uiGridSetVertAlignment (grid, row, col, alignment);

  return;
}

void
uiGridSetColumnSpacing (uiwcont_t *uigrid, int sz)
{
  IGrid         *grid;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-rm-row")) {
    return;
  }

  grid = uigrid->uidata.widget;
  grid.columnSpacing = uiBaseMarginSz * sz;
}

/* internal routines */

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

