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

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-free")) {
    return;
  }

  boxbase = &uibox->uiint.uiboxbase;

  wlist = boxbase->startlist;
  nlistFree (wlist);
  uibox->uiint.uiboxbase.startlist = NULL;

  wlist = boxbase->endlist;
  nlistFree (wlist);
  uibox->uiint.uiboxbase.endlist = NULL;
}

void
uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  uiboxbase_t *boxbase;
  nlist_t     *wlist;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start")) {
    return;
  }

  ruiBoxPackStart (uibox, uiwidget);

  boxbase = &uibox->uiint.uiboxbase;
  wlist = boxbase->startlist;
  boxbase->startcount += 1;
  nlistSetData (wlist, boxbase->startcount, uiwidget);
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  uiboxbase_t *boxbase;
  nlist_t     *wlist;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end")) {
    return;
  }

  ruiBoxPackEnd (uibox, uiwidget);

  boxbase = &uibox->uiint.uiboxbase;
  wlist = boxbase->endlist;
  boxbase->endcount += 1;
  nlistSetData (wlist, boxbase->endcount, uiwidget);
}

void
uiBoxPackStartExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  uiboxbase_t *boxbase;
  nlist_t     *wlist;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-ec")) {
    return;
  }

  ruiBoxPackStartExpandChildren (uibox, uiwidget);


  boxbase = &uibox->uiint.uiboxbase;
  wlist = boxbase->startlist;
  boxbase->startcount += 1;
  nlistSetData (wlist, boxbase->startcount, uiwidget);
}

void
uiBoxPackEndExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  uiboxbase_t *boxbase;
  nlist_t     *wlist;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end-ec")) {
    return;
  }

  ruiBoxPackEndExpandChildren (uibox, uiwidget);

  boxbase = &uibox->uiint.uiboxbase;
  wlist = boxbase->endlist;
  boxbase->endcount += 1;
  nlistSetData (wlist, boxbase->endcount, uiwidget);
}

/* internal routines */

static void
uiBoxCreateBase (uiwcont_t *uibox)
{
  uiboxbase_t   *boxbase;

  boxbase = &uibox->uiint.uiboxbase;
  boxbase->startlist = nlistAlloc ("box-start", LIST_ORDERED, NULL);
  boxbase->startcount = 0;
  boxbase->endlist = nlistAlloc ("box-end", LIST_ORDERED, NULL);
  boxbase->endcount = 0;
}

