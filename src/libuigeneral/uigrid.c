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

static void uiGridUpdateInternals (uiwcont_t *uigrid, uiwcont_t *uiwidget, uiwcontrls_t release);

uiwcont_t *
uiCreateGrid (void)
{
  uiwcont_t       *uigrid;
  uigridgeneric_t *uigridgen;

  uigrid = uiCreateGrid_r ();
  uigridgen = mdmalloc (sizeof (uigridgeneric_t));
  uigrid->uigridgen = uigridgen;

  uigridgen->widgetlist = nlistAlloc ("grid-widget", LIST_UNORDERED, NULL);
  uigridgen->releaselist = nlistAlloc ("grid-release", LIST_UNORDERED, NULL);
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
  nlistFree (uigridgen->widgetlist);
  nlistFree (uigridgen->releaselist);
  uigridgen->widgetlist = NULL;
  uigridgen->releaselist = NULL;
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
  uiGridUpdateInternals (uigrid, uiwidget, release);

  uigridgen->col += numcols;
  if (uigridgen->col > uigridgen->ncols) {
    uigridgen->ncols = uigridgen->col;
  }
  uigridgen->new = false;
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
  uiGridUpdateInternals (uigrid, uiwidget, release);

  uigridgen->row += 1;
  if (uigridgen->row > uigridgen->nrows) {
    uigridgen->nrows = uigridgen->row;
  }
  uigridgen->new = false;
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
  if (uigridgen->row > uigridgen->nrows) {
    uigridgen->nrows = uigridgen->row;
  }
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
  if (uigridgen->col > uigridgen->ncols) {
    uigridgen->ncols = uigridgen->col;
  }
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
}

void
uiGridPostProcess (uiwcont_t *uigrid)
{
  uigridgeneric_t *uigridgen;
  int         row, col;
  nlist_t     *wlist;
  nlist_t     *rlist;
  nlistidx_t  iteridx;
  nlistidx_t  key;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pp")) {
    return;
  }

  uigridgen = uigrid->uigridgen;
  row = 0;
  col = 0;

  if (uigridgen->postprocess) {
    logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
        "INFO: grid %" PRIu32 "-%" PRIu32 " twice", uigrid->ui_id, uigrid->id);
  }

  wlist = uigridgen->widgetlist;
  rlist = uigridgen->releaselist;
  nlistSort (wlist);
  nlistSort (rlist);

  nlistStartIterator (wlist, &iteridx);
  while ((key = nlistIterateKey (wlist, &iteridx)) >= 0) {
    uiwcont_t       *uiwidgetp;

    uiwidgetp = nlistGetData (wlist, key);
    if (uiwidgetp == NULL || uiwidgetp->widget == NULL) {
      break;
    }

    logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
        "grid-pp %" PRIu32 "-%" PRIu32,
        uigrid->ui_id, uigrid->id);
    uiGridPostProcess_r (uigrid, row, col, uiwidgetp);
    ++col;
    if (col >= uigridgen->ncols) {
      ++row;
      col = 0;
    }
    if (row >= uigridgen->nrows) {
      break;
    }
  }

  uigridgen->postprocess = true;
}

/* internal routines */

static void
uiGridUpdateInternals (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    uiwcontrls_t release)
{
  nlist_t     *wlist;
  nlist_t     *rlist;
  uigridgeneric_t *uigridgen;

  uigridgen = uigrid->uigridgen;
  wlist = uigridgen->widgetlist;
  rlist = uigridgen->releaselist;
  nlistSetData (wlist, uigridgen->count, uiwidget);
  nlistSetNum (rlist, uigridgen->count, release);
  uigridgen->count += 1;

  uiwcontSetID (uigrid, uiwidget, nlistGetCount (wlist));
  uiwidget->packed = true;
}
