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
uiGridPackNoExpand (uiwcont_t *uigrid, uiwcont_t *uiwidget)
{
  uigridint_t   *uigridint;
  IGrid         *grid;
  NSGridColumn  *col;
  NSGridCell    *cell;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  /* pack-size will update the column number, do this first */

  uigridint = uigrid->uiint.uigrid;
  grid = uigrid->uidata.widget;
fprintf (stderr, "grid: %d-%d p-no-exp: %d-%d %d/%s\n", uigrid->ui_id, uigrid->id, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  col = [grid columnAtIndex : uigridint->col];
  if (col == nil) {
    fprintf (stderr, "ERR: no column at %d\n", uigridint->col);
  } else {
    cell = [col cellAtIndex: uigridint->row];
    if (cell == nil) {
      fprintf (stderr, "ERR: no cell at %d/%d\n", uigridint->row, uigridint->col);
    } else {
      NSView    *view;

      view = uiwidget->uidata.widget;
      cell.xPlacement = NSGridCellPlacementLeading;
    }
  }

  uiGridPackSize (uigrid, uiwidget, 1, 1);
}

void
uiGridPackSize (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    int numrows, int numcols)
{
  uigridint_t   *uigridint;
  IGrid         *grid;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-size")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  uigridint = uigrid->uiint.uigrid;
  grid = uigrid->uidata.widget;

fprintf (stderr, "grid: %d-%d p: %d-%d %d/%s\n", uigrid->ui_id, uigrid->id, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
fprintf (stderr, "   col: %d row: %d\n", uigridint->col, uigridint->row);
fprintf (stderr, "   cols: %d rows: %d\n", grid.numberOfColumns, grid.numberOfRows);
  if (uigridint->col == 0) {
    NSView    *view;

    view = uiwidget->uidata.widget;
    [grid addRowWithViews : @[view]];
  } else {
    NSView        *view;

    view = uiwidget->uidata.widget;
    [grid addColumnWithViews : @[view]];
  }
  if (numcols > 1 || numrows > 1) {
    [grid
        mergeCellsInHorizontalRange : NSMakeRange (uigridint->row, numcols)
        verticalRange: NSMakeRange (uigridint->col, numrows)];
  }

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
