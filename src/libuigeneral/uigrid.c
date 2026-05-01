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

uiwcont_t *
uiCreateGrid (void)
{
  uiwcont_t     *uigrid;
  uigridint_t   *uigridint;

  uigrid = uiCreateGrid_r ();
  uigridint = mdmalloc (sizeof (uigridint_t));
  uigrid->uiint.uigridint = uigridint;

  uigridint->widgetlist = nlistAlloc ("grid-widget", LIST_UNORDERED, NULL);
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

  if (uigrid == NULL) {
    return;
  }

  uigridint = uigrid->uiint.uigridint;
  nlistFree (uigridint->widgetlist);
}

/* gtk3 defaults to fill mode, so create two routines to handle the pack */
void
uiGridPack (uiwcont_t *uigrid, uiwcont_t *uiwidget)
{
  if (uigrid == NULL || uiwidget == NULL) {
    return;
  }

  uiWidgetAlignHorizStart (uiwidget);
  uiGridPackSize (uigrid, uiwidget, 1);
}

void
uiGridPackFill (uiwcont_t *uigrid, uiwcont_t *uiwidget)
{
  if (uigrid == NULL || uiwidget == NULL) {
    return;
  }

  uiWidgetAlignHorizFill (uiwidget);
  uiGridPackSize (uigrid, uiwidget, 1);
}

void
uiGridPackSize (uiwcont_t *uigrid, uiwcont_t *uiwidget, int numcols)
{
  uigridint_t   *uigridint;
  nlist_t       *wlist;

  uiGridPackSize_r (uigrid, uiwidget, numcols);

  uigridint = uigrid->uiint.uigridint;

  wlist = uigridint->widgetlist;
  nlistSetData (wlist, uigridint->count, uiwidget);
  uigridint->count += 1;

  uigridint->col += numcols;
  uigridint->ncols = uigridint->col;
  uigridint->new = false;
}

void
uiGridNewRow (uiwcont_t *uigrid)
{
  uigridint_t    *uigridint;

  if (uigrid == NULL) {
    return;
  }

  uigridint = uigrid->uiint.uigridint;

  if (uigridint->new) {
    return;
  }

  uigridint->ncols = uigridint->col;
  uigridint->row += 1;
  uigridint->col = 0;
  uigridint->nrows = uigridint->row;
}

void
uiGridPostProcess (uiwcont_t *uigrid)
{
  uigridint_t *uigridint;
  int         row, col;
  nlist_t     *wlist;
  nlistidx_t  iteridx;
  nlistidx_t  key;

  if (! uiwcontValid (uigrid, WCONT_T_GRID, "grid-pp")) {
    return;
  }

  uigridint = uigrid->uiint.uigridint;
  row = 0;
  col = 0;

  if (uigridint->postprocess) {
    logMsg (LOG_DBG, LOG_UI, "INFO: grid %" PRIu32 "-%" PRIu32 " post-process twice\n", uigrid->ui_id, uigrid->id);
    // fprintf (stderr, "INFO: grid %" PRIu32 "-%" PRIu32 " post-process twice\n", uigrid->ui_id, uigrid->id);
  }

  wlist = uigridint->widgetlist;

  nlistStartIterator (wlist, &iteridx);
  while ((key = nlistIterateKey (wlist, &iteridx)) >= 0) {
    uiwcont_t       *uiwidgetp;

    uiwidgetp = nlistGetData (wlist, key);
    if (uiwidgetp == NULL || uiwidgetp->uidata.widget == NULL) {
      break;
    }

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

