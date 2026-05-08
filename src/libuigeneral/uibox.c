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
#include "nlist.h"
#include "tmutil.h"
#include "ui.h"

static void uiBoxCreateBase (uiwcont_t *uibox);
static void uiBoxStartListAdd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release);
static void uiBoxEndListAdd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release);
static void uiBoxListAdd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release, int32_t idx);

uiwcont_t *
uiCreateVertBox (void)
{
  uiwcont_t   *uibox;

  uibox = uiCreateVertBox_r ();
  uiBoxCreateBase (uibox);
  return uibox;
}

uiwcont_t *
uiCreateHorizBox (void)
{
  uiwcont_t   *uibox;

  uibox = uiCreateHorizBox_r ();
  uiBoxCreateBase (uibox);
  return uibox;
}

void
uiBoxFree (uiwcont_t *uibox)
{
  uiboxgeneric_t  *boxint;
  nlist_t     *wlist;
  nlist_t     *rlist;

  boxint = uibox->uiboxgen;
  wlist = boxint->widgetlist;
  rlist = boxint->releaselist;
  nlistFree (wlist);
  nlistFree (rlist);
  uibox->uiboxgen->widgetlist = NULL;
  uibox->uiboxgen->releaselist = NULL;
  mdfree (boxint);
  uiBoxFree_r (uibox);
}

void
uiBoxPostProcess (uiwcont_t *uibox)
{
  uiboxgeneric_t *boxint;
  nlist_t     *wlist;
  nlist_t     *rlist;
  nlistidx_t  iteridx;
  int         key;
  uiwcont_t   *prev = NULL;
  uiwcont_t   *curr = NULL;
  uiwcont_t   *next = NULL;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pp")) {
    return;
  }

  boxint = uibox->uiboxgen;

  if (boxint->postprocess) {
fprintf (stderr,
        "INFO: box %" PRIu32 "-%" PRIu32 " pp twice\n", uibox->ui_id, uibox->id);
    logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
        "INFO: box %" PRIu32 "-%" PRIu32 " pp twice", uibox->ui_id, uibox->id);
    return;
  }

  wlist = boxint->widgetlist;
  rlist = boxint->releaselist;
  nlistSort (wlist);
  nlistSort (rlist);

  nlistStartIterator (wlist, &iteridx);
  while ((key = nlistIterateKey (wlist, &iteridx)) >= 0) {
    uiwcont_t       *uiwidgetp;

    uiwidgetp = nlistGetData (wlist, key);
    if (uiwidgetp == NULL || uiwidgetp->widget == NULL) {
      break;
    }

    prev = curr;
    curr = next;
    next = uiwidgetp;
    if (curr != NULL) {
      logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
          "box-pp %" PRIu32 "-%" PRIu32 " %d/%s",
          uibox->ui_id, uibox->id, uibox->wtype, uiwcontDesc (uibox->wtype));
      uiBoxPostProcess_r (uibox, prev, curr, next, false);
    }
  }
  prev = curr;
  curr = next;
  next = NULL;
  logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
      "box-pp (last) %" PRIu32 "-%" PRIu32 " %d/%s",
      uibox->ui_id, uibox->id, uibox->wtype, uiwcontDesc
      (uibox->wtype));
  uiBoxPostProcess_r (uibox, prev, curr, next, true);

  boxint->postprocess = true;
}

void
uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  uiBoxStartListAdd (uibox, uiwidget, release);
  uiBoxPackStart_r (uibox, uiwidget);
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  uiBoxEndListAdd (uibox, uiwidget, release);
  uiBoxPackEnd_r (uibox, uiwidget);
}

void
uiBoxPackStartExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget,
    uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-ec")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  uiBoxStartListAdd (uibox, uiwidget, release);
  uiBoxPackStartExpandChildren_r (uibox, uiwidget);
}

void
uiBoxPackEndExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget,
    uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end-ec")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  uiBoxEndListAdd (uibox, uiwidget, release);
  uiBoxPackEndExpandChildren_r (uibox, uiwidget);
}

/* internal routines */

static void
uiBoxCreateBase (uiwcont_t *uibox)
{
  uiboxgeneric_t   *boxint;

  boxint = mdmalloc (sizeof (uiboxgeneric_t));
  uibox->uiboxgen = boxint;
  boxint->widgetlist = nlistAlloc ("box-widget", LIST_UNORDERED, NULL);
  boxint->releaselist = nlistAlloc ("box-release", LIST_UNORDERED, NULL);
  boxint->startcount = 0;
  boxint->endcount = 100;
  boxint->postprocess = false;
}

static void
uiBoxStartListAdd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  uiboxgeneric_t *boxint;

  boxint = uibox->uiboxgen;
  uiBoxListAdd (uibox, uiwidget, release, boxint->startcount);
  boxint->startcount += 1;
}

static void
uiBoxEndListAdd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  uiboxgeneric_t *boxint;

  boxint = uibox->uiboxgen;
  uiBoxListAdd (uibox, uiwidget, release, boxint->endcount);
  boxint->endcount -= 1;
}

static void
uiBoxListAdd (uiwcont_t *uibox, uiwcont_t *uiwidget,
    uiwcontrls_t release, int32_t idx)
{
  uiboxgeneric_t  *boxint;
  nlist_t     *wlist;
  nlist_t     *rlist;


  boxint = uibox->uiboxgen;
  wlist = boxint->widgetlist;
  nlistSetData (wlist, idx, uiwidget);
  rlist = boxint->releaselist;
  nlistSetNum (rlist, idx, release);

  uiwcontSetID (uibox, uiwidget, nlistGetCount (wlist));
  logMsg (LOG_DBG, LOG_UI | LOG_BACKTRACE,
      "wcont: pack: %" PRIu32 "-%" PRIu32 " %s box: %" PRIu32 "-%" PRIu32,
      uiwidget->ui_id, uiwidget->id, uiwcontDesc (uiwidget->wtype),
      uibox->ui_id, uibox->id);
  uiwidget->parentid = uibox->id;
  uiwidget->packed = true;
}
