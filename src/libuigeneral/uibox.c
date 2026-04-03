/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "log.h"
#include "tmutil.h"
#include "uigeneral.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"

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
  uiboxbase_t *boxbase;
  nlist_t     *wlist;
  nlist_t     *rlist;
  nlistidx_t  iteridx;
  int         key;
  int32_t     tid;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-free")) {
    return;
  }

  boxbase = &uibox->uiint.uiboxbase;
  tid = uibox->id;

  if (! boxbase->postprocess) {
    fprintf (stderr, "ERR: box %d not post-process\n", tid);
  }

  wlist = boxbase->widgetlist;
  rlist = boxbase->releaselist;
  nlistStartIterator (wlist, &iteridx);
  while ((key = nlistIterateKey (wlist, &iteridx)) >= 0) {
    uiwcontrls_t    release;

    release = nlistGetNum (rlist, key);
    if (release == WCONT_FREE) {
      uiwcont_t     *uiwidgetp;

      uiwidgetp = nlistGetData (wlist, key);
      uiwcontFree (uiwidgetp);
    }
  }
  nlistFree (wlist);
  nlistFree (rlist);
  uibox->uiint.uiboxbase.widgetlist = NULL;
  uibox->uiint.uiboxbase.releaselist = NULL;
}

void
uiBoxPostProcess (uiwcont_t *uibox)
{
  uiboxbase_t *boxbase;
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

  boxbase = &uibox->uiint.uiboxbase;

  if (boxbase->postprocess) {
// ### remove this later
    fprintf (stderr, "INFO: box %d post-process twice\n", uibox->id);
  }

  wlist = boxbase->widgetlist;
  rlist = boxbase->releaselist;
  nlistSort (wlist);
  nlistSort (rlist);

  nlistStartIterator (wlist, &iteridx);
  while ((key = nlistIterateKey (wlist, &iteridx)) >= 0) {
    uiwcont_t       *uiwidgetp;

    uiwidgetp = nlistGetData (wlist, key);
    if (uiwidgetp == NULL || uiwidgetp->uidata.widget == NULL) {
      break;
    }

    prev = curr;
    curr = next;
    next = uiwidgetp;
    if (curr != NULL) {
      uiBoxPostProcess_r (uibox, prev, curr, next);
    }
  }
  prev = curr;
  curr = next;
  next = NULL;
  uiBoxPostProcess_r (uibox, prev, curr, next);

  boxbase->postprocess = true;
}

void
uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  uiBoxPackStart_r (uibox, uiwidget);
  uiBoxStartListAdd (uibox, uiwidget, release);
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  uiBoxPackEnd_r (uibox, uiwidget);
  uiBoxEndListAdd (uibox, uiwidget, release);
}

void
uiBoxPackStartExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget,
    uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-ec")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  uiBoxPackStartExpandChildren_r (uibox, uiwidget);
  uiBoxStartListAdd (uibox, uiwidget, release);
}

void
uiBoxPackEndExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget,
    uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end-ec")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  uiBoxPackEndExpandChildren_r (uibox, uiwidget);
  uiBoxEndListAdd (uibox, uiwidget, release);
}

/* internal routines */

static void
uiBoxCreateBase (uiwcont_t *uibox)
{
  uiboxbase_t   *boxbase;

  boxbase = &uibox->uiint.uiboxbase;
  boxbase->widgetlist = nlistAlloc ("box-widget", LIST_UNORDERED, NULL);
  boxbase->releaselist = nlistAlloc ("box-release", LIST_UNORDERED, NULL);
  boxbase->startcount = 0;
  boxbase->endcount = 100;
  boxbase->postprocess = false;
}

static void
uiBoxStartListAdd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  uiboxbase_t *boxbase;

  boxbase = &uibox->uiint.uiboxbase;
  uiBoxListAdd (uibox, uiwidget, release, boxbase->startcount);
  boxbase->startcount += 1;
}

static void
uiBoxEndListAdd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  uiboxbase_t *boxbase;

  boxbase = &uibox->uiint.uiboxbase;
  uiBoxListAdd (uibox, uiwidget, release, boxbase->endcount);
  boxbase->endcount -= 1;
}

static void
uiBoxListAdd (uiwcont_t *uibox, uiwcont_t *uiwidget,
    uiwcontrls_t release, int32_t idx)
{
  uiboxbase_t *boxbase;
  nlist_t     *wlist;
  nlist_t     *rlist;


  boxbase = &uibox->uiint.uiboxbase;
  wlist = boxbase->widgetlist;
  nlistSetData (wlist, idx, uiwidget);
  rlist = boxbase->releaselist;
  nlistSetNum (rlist, idx, release);

  if (uiwidget->wbasetype != WCONT_T_BOX &&
      uiwidget->wbasetype != WCONT_T_WINDOW) {
    uiwidget->id = uibox->id + nlistGetCount (wlist);
  }
fprintf (stderr, "wcont: pack: %d %s box: %d\n", uiwidget->id, uiwcontDesc (uiwidget->wtype), uibox->id);
  uiwidget->parentid = uibox->id;
  uiwidget->packed = true;
}
