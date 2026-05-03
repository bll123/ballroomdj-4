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
#include "uigeneral.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uigrid.h"
#include "ui/uiwidget.h"

static void uiGridUpdateInternals (uiwcont_t *uigrid, uiwcont_t *uiwidget, uiwcontrls_t release);

uiwcont_t *
uiCreateGrid (void)
{
  uiwcont_t     *uigrid;
  uigridint_t   *uigridint;

  uigrid = uiCreateGrid_r ();
  uigridint = mdmalloc (sizeof (uigridint_t));
  uigrid->uiint.uigridint = uigridint;

  uigridint->widgetlist = nlistAlloc ("grid-widget", LIST_UNORDERED, NULL);
  uigridint->releaselist = nlistAlloc ("grid-release", LIST_UNORDERED, NULL);
  uigridint->count = 0;
  uigridint->nrows = 0;
  uigridint->ncols = 0;
  uigridint->row = 0;
  uigridint->col = 0;
  uigridint->new = true;
  uigridint->postprocess = false;

  return uigrid;
}

void
uiGridFree (uiwcont_t *uigrid)
{
  uigridint_t   *uigridint;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-free")) {
    return;
  }

  uigridint = uigrid->uiint.uigridint;
  nlistFree (uigridint->widgetlist);
  nlistFree (uigridint->releaselist);
  uigridint->widgetlist = NULL;
  uigridint->releaselist = NULL;
  mdfree (uigridint);
  uigrid->uiint.uigridint = NULL;
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
  uigridint_t   *uigridint;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-column")) {
    return;
  }
  if (uiwidget == NULL) {
    return;
  }

  uigridint = uigrid->uiint.uigridint;

  logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
      "  grid-pack-c %" PRIu32 "-%" PRIu32 " %d/%d : %" PRIu32 "-%" PRIu32 " %d/%s",
      uigrid->ui_id, uigrid->id, uigridint->row, uigridint->col,
      uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  uiWidgetAlignHorizStart (uiwidget);
  uiGridPackLoc_r (uigrid, uiwidget, uigridint->row, uigridint->col, numcols);
  uiGridUpdateInternals (uigrid, uiwidget, release);

  uigridint->col += numcols;
  if (uigridint->col > uigridint->ncols) {
    uigridint->ncols = uigridint->col;
  }
  uigridint->new = false;
}

void
uiGridPackRow (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    int numcols, uiwcontrls_t release)
{
  uigridint_t   *uigridint;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-row")) {
    return;
  }
  if (uiwidget == NULL) {
    return;
  }

  uigridint = uigrid->uiint.uigridint;

  uiWidgetAlignHorizStart (uiwidget);
  logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
      "  grid-pack-r %" PRIu32 "-%" PRIu32 " %d/%d : %" PRIu32 "-%" PRIu32 " %d/%s",
      uigrid->ui_id, uigrid->id, uigridint->row, uigridint->col,
      uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  uiGridPackLoc_r (uigrid, uiwidget, uigridint->row, uigridint->col, numcols);
  uiGridUpdateInternals (uigrid, uiwidget, release);

  uigridint->row += 1;
  if (uigridint->row > uigridint->nrows) {
    uigridint->nrows = uigridint->row;
  }
  uigridint->new = false;
}

void
uiGridNewRow (uiwcont_t *uigrid)
{
  uigridint_t    *uigridint;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-row")) {
    return;
  }

  uigridint = uigrid->uiint.uigridint;

  if (uigridint->new) {
    return;
  }

  uigridint->row += 1;
  uigridint->col = 0;
  if (uigridint->row > uigridint->nrows) {
    uigridint->nrows = uigridint->row;
  }
}

void
uiGridNewColumn (uiwcont_t *uigrid)
{
  uigridint_t    *uigridint;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pack-row")) {
    return;
  }

  uigridint = uigrid->uiint.uigridint;

  if (uigridint->new) {
    return;
  }

  uigridint->row = 0;
  uigridint->col += 1;
  if (uigridint->col > uigridint->ncols) {
    uigridint->ncols = uigridint->col;
  }
}

void
uiGridPostProcess (uiwcont_t *uigrid)
{
  uigridint_t *uigridint;
  int         row, col;
  nlist_t     *wlist;
  nlist_t     *rlist;
  nlistidx_t  iteridx;
  nlistidx_t  key;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pp")) {
    return;
  }

  uigridint = uigrid->uiint.uigridint;
  row = 0;
  col = 0;

  if (uigridint->postprocess) {
    logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
        "INFO: grid %" PRIu32 "-%" PRIu32 " twice", uigrid->ui_id, uigrid->id);
  }

  wlist = uigridint->widgetlist;
  rlist = uigridint->releaselist;
  nlistSort (wlist);
  nlistSort (rlist);

  nlistStartIterator (wlist, &iteridx);
  while ((key = nlistIterateKey (wlist, &iteridx)) >= 0) {
    uiwcont_t       *uiwidgetp;

    uiwidgetp = nlistGetData (wlist, key);
    if (uiwidgetp == NULL || uiwidgetp->uidata.widget == NULL) {
      break;
    }

    logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
        "grid-pp %" PRIu32 "-%" PRIu32,
        uigrid->ui_id, uigrid->id);
    uiGridPostProcess_r (uigrid, row, col, uiwidgetp);
    ++col;
    if (col >= uigridint->ncols) {
      ++row;
      col = 0;
    }
    if (row >= uigridint->nrows) {
      break;
    }
  }

  uigridint->postprocess = true;
}

/* internal routines */

static void
uiGridUpdateInternals (uiwcont_t *uigrid, uiwcont_t *uiwidget,
    uiwcontrls_t release)
{
  nlist_t     *wlist;
  nlist_t     *rlist;
  uigridint_t *uigridint;

  uigridint = uigrid->uiint.uigridint;
  wlist = uigridint->widgetlist;
  rlist = uigridint->releaselist;
  nlistSetData (wlist, uigridint->count, uiwidget);
  nlistSetNum (rlist, uigridint->count, release);
  uigridint->count += 1;

  uiwcontSetID (uigrid, uiwidget, nlistGetCount (wlist));
  uiwidget->packed = true;
}
