/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "ilist.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "ui.h"
#include "callback.h"
#include "uidance.h"
#include "uidd.h"

typedef struct uidance {
  dance_t       *dances;
  uiwcont_t     *parentwin;
  uidd_t        *uidd;
  callback_t    *internalselcb;
  callback_t    *selectcb;
  const char    *label;
  ilist_t       *ddlist;
  ilistidx_t    selectedidx;
  int           count;
  int           flags;
} uidance_t;

static bool uidanceSelectHandler (void *udata, int32_t dkey);
static void uidanceCreateDanceList (uidance_t *uidance);

uidance_t *
uidanceCreate (uiwcont_t *boxp, uiwcont_t *parentwin, int flags,
    const char *label, int where, int count)
{
  uidance_t   *uidance;
  int         ddwhere = DD_PACK_START;
  int         ddflag;

  uidance = mdmalloc (sizeof (uidance_t));
  uidance->dances = bdjvarsdfGet (BDJVDF_DANCES);
  uidance->count = count;
  uidance->flags = flags;
  uidance->selectedidx = 0;
  uidance->parentwin = parentwin;
  uidance->internalselcb = NULL;
  uidance->selectcb = NULL;
  uidance->ddlist = NULL;

  uidance->internalselcb = callbackInitI (uidanceSelectHandler, uidance);

  if (where == UIDANCE_PACK_END) {
    ddwhere = DD_PACK_END;
  }
  if (where == UIDANCE_PACK_START) {
    ddwhere = DD_PACK_START;
  }
  uidance->label = label;
  uidanceCreateDanceList (uidance);
  ddflag = DD_REPLACE_TITLE;
  if (flags == UIDANCE_NONE) {
    ddflag = DD_KEEP_TITLE;
  }
  uidance->uidd = uiddCreate ("uidance", parentwin, boxp, ddwhere,
      uidance->ddlist, DD_LIST_TYPE_NUM,
      label, ddflag, uidance->internalselcb);

  return uidance;
}

void
uidanceFree (uidance_t *uidance)
{
  if (uidance == NULL) {
    return;
  }

  callbackFree (uidance->internalselcb);
  uiddFree (uidance->uidd);
  ilistFree (uidance->ddlist);
  mdfree (uidance);
}

int
uidanceGetKey (uidance_t *uidance)
{
  if (uidance == NULL) {
    return 0;
  }

  return uidance->selectedidx;
}

void
uidanceSetKey (uidance_t *uidance, ilistidx_t dkey)
{
  if (uidance == NULL || uidance->uidd == NULL) {
    return;
  }

  uidance->selectedidx = dkey;
  uiddSetSelectionByNumKey (uidance->uidd, dkey);
}

void
uidanceSetState (uidance_t *uidance, int state)
{
  if (uidance == NULL || uidance->uidd == NULL) {
    return;
  }
  uiddSetState (uidance->uidd, state);
}

void
uidanceSetCallback (uidance_t *uidance, callback_t *cb)
{
  if (uidance == NULL) {
    return;
  }
  uidance->selectcb = cb;
}


/* internal routines */

static bool
uidanceSelectHandler (void *udata, int32_t dkey)
{
  uidance_t   *uidance = udata;

  uidance->selectedidx = dkey;

  if (uidance->selectcb != NULL) {
    callbackHandlerII (uidance->selectcb, dkey, uidance->count);
  }
  return UICB_CONT;
}

static void
uidanceCreateDanceList (uidance_t *uidance)
{
  slist_t     *danceList;
  slistidx_t  iteridx;
  ilist_t     *ddlist;
  const char  *disp;
  int         count;
  ilistidx_t  dkey;
  int         idx;

  danceList = danceGetDanceList (uidance->dances);
  count = slistGetCount (danceList);
  ddlist = ilistAlloc ("uidance", LIST_ORDERED);
  ilistSetSize (ddlist, count);

  idx = 0;
  /* if it is a combobox (UIDANCE_ALL_DANCES, UIDANCE_EMPTY_DANCE) */
  if (uidance->flags != UIDANCE_NONE) {
    ilistSetNum (ddlist, idx, DD_LIST_KEY_NUM, DD_NO_SELECTION);
    ilistSetStr (ddlist, idx, DD_LIST_DISP, uidance->label);
    ++idx;
  }

  slistStartIterator (danceList, &iteridx);
  while ((disp = slistIterateKey (danceList, &iteridx)) != NULL) {
    dkey = slistGetNum (danceList, disp);
    ilistSetNum (ddlist, idx, DD_LIST_KEY_NUM, dkey);
    ilistSetStr (ddlist, idx, DD_LIST_DISP, disp);
    ++idx;
  }


  uidance->ddlist = ddlist;
}

