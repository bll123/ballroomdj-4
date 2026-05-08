/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "callback.h"
#include "log.h"
#include "mdebug.h"
#include "tmutil.h"
#include "ui.h"

static void uiGridUpdateInternals (uiwcont_t *uigrid, uiwcont_t *uiwidget, int row, int col, int numcols, uiwcontrls_t release);
void uiGridUpdateSize (uiwcont_t *uigrid, int numcols);

uiwcont_t *
uiCreateGrid (void)
{
  uiwcont_t       *uigrid;
  uigridgeneric_t *uigridgen;

  uigrid = uiCreateGrid_r ();
  uigridgen = mdmalloc (sizeof (uigridgeneric_t));
  uigrid->uigridgen = uigridgen;

  uigridgen->widgetlist = ilistAlloc ("grid-widget", LIST_ORDERED);
  uigridgen->count = 0;
  uigridgen->nrows = 0;
  uigridgen->ncols = 0;
  uigridgen->row = 0;
  uigridgen->col = 0;
  uigridgen->new = true;
  uigridgen->postprocess = false;

  return uigrid;
}

void
uiGridFree (uiwcont_t *uigrid)
{
  uigridgeneric_t   *uigridgen;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-free")) {
    return;
  }

  uigridgen = uigrid->uigridgen;
  ilistFree (uigridgen->widgetlist);
  uigridgen->widgetlist = NULL;
  mdfree (uigridgen);
  uigrid->uigridgen = NULL;
}

void
uiGridPack (uiwcont_t *uigrid, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  uiGridPackColumn (uigrid, uiwidget, 1, release);
}

void
uiGridPackColumn (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    int numcols, uiwcontrls_t release)
{
  uigridgeneric_t   *uigridgen;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-column")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->packwidget == NULL) {
    return;
  }

  uigridgen = uigrid->uigridgen;

  logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
      "  grid-pack-c %" PRIu32 "-%" PRIu32 " %d/%d : %" PRIu32 "-%" PRIu32 " %d/%s",
      uigrid->ui_id, uigrid->id, uigridgen->row, uigridgen->col,
      uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  uiWidgetAlignHorizStart (uiwidget);
  uiGridPackLoc_r (uigrid, uiwidget, uigridgen->row, uigridgen->col, numcols);
  uiGridUpdateInternals (uigrid, uiwidget,
      uigridgen->row, uigridgen->col, numcols, release);

  uiGridUpdateSize (uigrid, numcols);
  uigridgen->col += numcols;
  uigridgen->new = false;
//fprintf (stderr, "grid-pc: %d-%d r:%d c:%d nc:%d  nrows:%d ncols:%d\n", uigrid->ui_id, uigrid->id, uigridgen->row, uigridgen->col, numcols, uigridgen->nrows, uigridgen->ncols);
}

void
uiGridPackRow (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    int numcols, uiwcontrls_t release)
{
  uigridgeneric_t   *uigridgen;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-row")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->packwidget == NULL) {
    return;
  }

  uigridgen = uigrid->uigridgen;

  uiWidgetAlignHorizStart (uiwidget);
  logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
      "  grid-pack-r %" PRIu32 "-%" PRIu32 " %d/%d : %" PRIu32 "-%" PRIu32 " %d/%s",
      uigrid->ui_id, uigrid->id, uigridgen->row, uigridgen->col,
      uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  uiGridPackLoc_r (uigrid, uiwidget, uigridgen->row, uigridgen->col, numcols);
  uiGridUpdateInternals (uigrid, uiwidget,
      uigridgen->row, uigridgen->col, numcols, release);

  uiGridUpdateSize (uigrid, numcols);
  uigridgen->row += 1;
  uigridgen->new = false;
//fprintf (stderr, "grid-pr: %d-%d r:%d c:%d nrows:%d ncols:%d\n", uigrid->ui_id, uigrid->id, uigridgen->row, uigridgen->col, uigridgen->nrows, uigridgen->ncols);
}

void
uiGridNewRow (uiwcont_t *uigrid)
{
  uigridgeneric_t    *uigridgen;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-row")) {
    return;
  }

  uigridgen = uigrid->uigridgen;

  if (uigridgen->new) {
    return;
  }

  uigridgen->row += 1;
  uigridgen->col = 0;
//fprintf (stderr, "grid-nr: %d-%d r:%d c:%d nrows:%d ncols:%d\n", uigrid->ui_id, uigrid->id, uigridgen->row, uigridgen->col, uigridgen->nrows, uigridgen->ncols);
}

void
uiGridNewColumn (uiwcont_t *uigrid)
{
  uigridgeneric_t    *uigridgen;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-row")) {
    return;
  }

  uigridgen = uigrid->uigridgen;

  if (uigridgen->new) {
    return;
  }

  uigridgen->row = 0;
  uigridgen->col += 1;
//fprintf (stderr, "grid-nc: %d-%d r:%d c:%d nrows:%d ncols:%d\n", uigrid->ui_id, uigrid->id, uigridgen->row, uigridgen->col, uigridgen->nrows, uigridgen->ncols);
}

void
uiGridSkipColumn (uiwcont_t *uigrid, int numcols)
{
  uigridgeneric_t    *uigridgen;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-row")) {
    return;
  }

  uigridgen = uigrid->uigridgen;

  if (uigridgen->new) {
    return;
  }

  uigridgen->col += numcols;
  if (uigridgen->col > uigridgen->ncols) {
    uigridgen->ncols = uigridgen->col;
  }
  uigridgen->new = false;
//fprintf (stderr, "grid-sc: %d-%d r:%d c:%d nrows:%d ncols:%d\n", uigrid->ui_id, uigrid->id, uigridgen->row, uigridgen->col, uigridgen->nrows, uigridgen->ncols);
}

void
uiGridPostProcess (uiwcont_t *uigrid)
{
  uigridgeneric_t *uigridgen;
  int         row, col;
  ilist_t     *wlist;
  nlistidx_t  iteridx;
  nlistidx_t  key;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pp")) {
    return;
  }

  uigridgen = uigrid->uigridgen;
  row = 0;
  col = 0;

  if (uigridgen->postprocess) {
fprintf (stderr,
        "INFO: grid %" PRIu32 "-%" PRIu32 " pp twice\n", uigrid->ui_id, uigrid->id);
    logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
        "INFO: grid %" PRIu32 "-%" PRIu32 " pp twice", uigrid->ui_id, uigrid->id);
    return;
  }

fprintf (stderr, "grid-pp: %d-%d nrows:%d ncols:%d\n", uigrid->ui_id, uigrid->id, uigridgen->nrows, uigridgen->ncols);
  wlist = uigridgen->widgetlist;

  nlistStartIterator (wlist, &iteridx);
  while ((key = ilistIterateKey (wlist, &iteridx)) >= 0) {
    uiwcont_t       *uiwidgetp;
    int             row, col;
    int             numcols;

    uiwidgetp = ilistGetData (wlist, key, UIGRID_WIDGET);
    if (uiwidgetp == NULL || uiwidgetp->widget == NULL) {
      continue;
    }

    row = ilistGetNum (wlist, key, UIGRID_ROW);
    col = ilistGetNum (wlist, key, UIGRID_COL);
    numcols = ilistGetNum (wlist, key, UIGRID_NUMCOLS);

    logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
        "grid-pp %" PRIu32 "-%" PRIu32,
        uigrid->ui_id, uigrid->id);
    uiGridPostProcess_r (uigrid, row, col, numcols, uiwidgetp);
  }

  uigridgen->postprocess = true;
}

/* internal routines */

static void
uiGridUpdateInternals (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    int row, int col, int numcols, uiwcontrls_t release)
{
  ilist_t         *wlist;
  uigridgeneric_t *uigridgen;

  uigridgen = uigrid->uigridgen;
  wlist = uigridgen->widgetlist;
  ilistSetData (wlist, uigridgen->count, UIGRID_WIDGET, uiwidget);
  ilistSetNum (wlist, uigridgen->count, UIGRID_ROW, row);
  ilistSetNum (wlist, uigridgen->count, UIGRID_COL, col);
  ilistSetNum (wlist, uigridgen->count, UIGRID_NUMCOLS, numcols);
  ilistSetNum (wlist, uigridgen->count, UIGRID_RLS_TYPE, release);
  uigridgen->count += 1;

  uiwcontSetID (uigrid, uiwidget, nlistGetCount (wlist));
  uiwidget->packed = true;
}

/* call this before updating row/col */
void
uiGridUpdateSize (uiwcont_t *uigrid, int numcols)
{
  uigridgeneric_t   *uigridgen;

  uigridgen = uigrid->uigridgen;
  /* row/col point to the current grid cell that was filled */
  /* a) 0/0 -> 1/1 one col */
  /*    1/0 -> 2/1 */
  /*    2/0 -> 3/1 */
  /* b) 0/0 -> 1/1 three col */
  /*    0/1 -> 1/2 */
  /*    0/2 -> 1/3 */
  /*    1/0 -> 2/3 */
  /* c) 0/0 -> 1/1 pack-row */
  /*    1/0 -> 2/1 */
  /*    2/0 -> 3/1 */
  /*    0/1 -> 3/2 */
  /*    1/1 -> 3/2 */
  if (uigridgen->nrows == 0) {
    uigridgen->nrows = 1;
  }
  if (uigridgen->ncols == 0) {
    uigridgen->ncols = 1;
  }
  if (uigridgen->col + numcols >= uigridgen->ncols) {
    uigridgen->ncols = uigridgen->col + numcols;
  }
  if (uigridgen->row + 1 >= uigridgen->nrows) {
    uigridgen->nrows = uigridgen->row + 1;
  }
fprintf (stderr, "grid-us: %d-%d r:%d c:%d nc:%d  nrows:%d ncols:%d\n", uigrid->ui_id, uigrid->id, uigridgen->row, uigridgen->col, numcols, uigridgen->nrows, uigridgen->ncols);
}
