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

#include "ui/uiwcont-int.h"

#include "ui/uigrid.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"

@interface IGrid : NSGridView {}
- (BOOL) isFlipped;
@end

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
  uigrid_t    *uigrid;

  uigrid = mdmalloc (sizeof (uigrid_t));
  uigrid->row = 0;
  uigrid->col = 0;
  uigrid->new = true;

  grid = [[IGrid alloc] init];
  grid.columnSpacing = uiBaseMarginSz * 2;

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
  uigrid_t    *uigrid;

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
  uigrid_t      *uigridint;
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
  uigrid_t      *uigridint;
  IGrid         *grid;
  NSGridColumn  *col;
  NSGridCell    *cell;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-size")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.packwidget == NULL) {
    return;
  }

  uigridint = uigrid->uiint.uigrid;
  grid = uigrid->uidata.widget;

  if (uigridint->col == 0) {
    NSView    *view;
//    NSGridRow *row;

    view = uiwidget->uidata.widget;
    [grid addRowWithViews : @[view]];
//    row = [grid rowAtIndex : uigridint->row];
//    row.
  } else {
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
        cell.contentView = view;
      }
    }
  }
  if (numcols > 1 || numrows > 1) {
    [grid
        mergeCellsInHorizontalRange : NSMakeRange (uigridint->row, numcols)
        verticalRange: NSMakeRange (uigridint->col, numrows)];
  }

  uigridint->col += 1;
  uigridint->new = false;
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
