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

static int32_t  gboxcount = 0;

uiwcont_t *
uiCreateVertBox (void)
{
  uiwcont_t   *uibox;

  uibox = ruiCreateVertBox ();
  uiBoxCreateBase (uibox);
  return uibox;
}

uiwcont_t *
uiCreateHorizBox (void)
{
  uiwcont_t   *uibox;

  uibox = ruiCreateHorizBox ();
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
  tid = boxbase->id;

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
uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  nuiBoxPackStart (uibox, uiwidget, WCONT_KEEP);
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  nuiBoxPackEnd (uibox, uiwidget, WCONT_KEEP);
}

void
uiBoxPackStartExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  nuiBoxPackStartExpandChildren (uibox, uiwidget, WCONT_KEEP);
}

void
uiBoxPackEndExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  nuiBoxPackEndExpandChildren (uibox, uiwidget, WCONT_KEEP);
}

void
nuiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  ruiBoxPackStart (uibox, uiwidget);
  uiBoxStartListAdd (uibox, uiwidget, release);
}

void
nuiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  ruiBoxPackEnd (uibox, uiwidget);
  uiBoxEndListAdd (uibox, uiwidget, release);
}

void
nuiBoxPackStartExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget,
    uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-ec")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  ruiBoxPackStartExpandChildren (uibox, uiwidget);
  uiBoxStartListAdd (uibox, uiwidget, release);
}

void
nuiBoxPackEndExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget,
    uiwcontrls_t release)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end-ec")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  ruiBoxPackEndExpandChildren (uibox, uiwidget);
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
  boxbase->id = gboxcount;
  boxbase->indent = 0;
  boxbase->parentid = 0;
  gboxcount += 1;
}

static void
uiBoxStartListAdd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  uiboxbase_t *boxbase;
  nlist_t     *wlist;
  nlist_t     *rlist;


  boxbase = &uibox->uiint.uiboxbase;
  wlist = boxbase->widgetlist;
  nlistSetData (wlist, boxbase->startcount, uiwidget);
  rlist = boxbase->releaselist;
  nlistSetNum (rlist, boxbase->startcount, release);
  boxbase->startcount += 1;

  if (uiwidget->wbasetype == WCONT_T_BOX) {
    uiboxbase_t *tboxbase;

    tboxbase = &uiwidget->uiint.uiboxbase;
    tboxbase->parentid = boxbase->id;
    tboxbase->indent = boxbase->indent + 3;
  }
}

static void
uiBoxEndListAdd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release)
{
  uiboxbase_t *boxbase;
  nlist_t     *wlist;
  nlist_t     *rlist;


  boxbase = &uibox->uiint.uiboxbase;
  wlist = boxbase->widgetlist;
  nlistSetData (wlist, boxbase->endcount, uiwidget);
  rlist = boxbase->releaselist;
  nlistSetNum (rlist, boxbase->endcount, release);
  boxbase->endcount -= 1;
}
