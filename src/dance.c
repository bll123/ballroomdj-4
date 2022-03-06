#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "datafile.h"
#include "dnctypes.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "slist.h"

static void danceConvSpeed (char *keydata, datafileret_t *ret);

  /* must be sorted in ascii order */
static datafilekey_t dancedfkeys[] = {
  { "ANNOUNCE",   DANCE_ANNOUNCE, VALUE_DATA, NULL, -1 },
  { "COUNT",      DANCE_COUNT, VALUE_NUM, NULL, -1 },
  { "DANCE",      DANCE_DANCE, VALUE_DATA, NULL, -1 },
  { "HIGHBPM",    DANCE_HIGH_BPM, VALUE_NUM, NULL, -1 },
  { "LOWBPM",     DANCE_LOW_BPM, VALUE_NUM, NULL, -1 },
  { "SELECT",     DANCE_SELECT, VALUE_NUM, NULL, -1 },
  { "SPEED",      DANCE_SPEED, VALUE_LIST, danceConvSpeed, -1 },
  { "TAGS",       DANCE_TAGS, VALUE_LIST, parseConvTextList, -1 },
  { "TIMESIG",    DANCE_TIMESIG, VALUE_DATA, NULL, -1 },
  { "TYPE",       DANCE_TYPE, VALUE_DATA, dnctypesConv, -1 },
};
#define DANCE_DFKEY_COUNT (sizeof (dancedfkeys) / sizeof (datafilekey_t))

static datafilekey_t dancespeeddfkeys[] = {
  { "fast",       DANCE_SPEED_FAST,   VALUE_DATA, NULL, -1 },
  { "normal",     DANCE_SPEED_NORMAL, VALUE_DATA, NULL, -1 },
  { "slow",       DANCE_SPEED_SLOW,   VALUE_DATA, NULL, -1 },
};
#define DANCE_SPEED_DFKEY_COUNT (sizeof (dancespeeddfkeys) / sizeof (datafilekey_t))

dance_t *
danceAlloc (char *fname)
{
  dance_t           *dance;

  if (! fileopFileExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "dance: missing: %s", fname);
    return NULL;
  }

  dance = malloc (sizeof (dance_t));
  assert (dance != NULL);
  dance->danceList = NULL;

  dance->df = datafileAllocParse ("dance", DFTYPE_INDIRECT, fname,
      dancedfkeys, DANCE_DFKEY_COUNT, DANCE_DANCE);
  dance->dances = datafileGetList (dance->df);
  return dance;
}

void
danceFree (dance_t *dance)
{
  if (dance != NULL) {
    if (dance->df != NULL) {
      datafileFree (dance->df);
    }
    if (dance->danceList != NULL) {
      slistFree (dance->danceList);
    }
    free (dance);
  }
}

void
danceStartIterator (dance_t *dances, ilistidx_t *iteridx)
{
  if (dances == NULL || dances->dances == NULL) {
    return;
  }

  ilistStartIterator (dances->dances, iteridx);
}

ilistidx_t
danceIterateKey (dance_t *dances, ilistidx_t *iteridx)
{
  ilistidx_t     ikey;

  if (dances == NULL || dances->dances == NULL) {
    return LIST_LOC_INVALID;
  }

  ikey = ilistIterateKey (dances->dances, iteridx);
  return ikey;
}

slist_t *
danceGetLookup (void)
{
  dance_t *dance = bdjvarsdfGet (BDJVDF_DANCES);
  slist_t *lookup = datafileGetLookup (dance->df);
  return lookup;
}

ssize_t
danceGetCount (dance_t *dance)
{
  return ilistGetCount (dance->dances);
}

char *
danceGetData (dance_t *dance, ilistidx_t dkey, ilistidx_t idx)
{
  return ilistGetData (dance->dances, dkey, idx);
}

slist_t *
danceGetList (dance_t *dance, ilistidx_t dkey, ilistidx_t idx)
{
  return ilistGetList (dance->dances, dkey, idx);
}

ssize_t
danceGetNum (dance_t *dance, ilistidx_t dkey, ilistidx_t idx)
{
  return ilistGetNum (dance->dances, dkey, idx);
}

slist_t *
danceGetDanceList (dance_t *dance)
{
  slist_t     *dl;
  ilistidx_t  key;
  char        *nm;
  ilistidx_t  iteridx;

  if (dance->danceList != NULL) {
    return dance->danceList;
  }

  dl = slistAlloc ("dancelist", LIST_UNORDERED, NULL);
  slistSetSize (dl, ilistGetCount (dance->dances));
  ilistStartIterator (dance->dances, &iteridx);
  while ((key = ilistIterateKey (dance->dances, &iteridx)) >= 0) {
    nm = ilistGetData (dance->dances, key, DANCE_DANCE);
    slistSetNum (dl, nm, key);
  }
  slistSort (dl);

  dance->danceList = dl;
  return dl;
}

void
danceConvDance (char *keydata, datafileret_t *ret)
{
  dance_t       *dance;
  slist_t       *lookup;

  ret->valuetype = VALUE_NUM;
  ret->u.num = -1;
  dance = bdjvarsdfGet (BDJVDF_DANCES);
  lookup = datafileGetLookup (dance->df);
  if (lookup != NULL) {
    ret->u.num = slistGetNum (lookup, keydata);
  }
}

/* internal routines */

static void
danceConvSpeed (char *keydata, datafileret_t *ret)
{
  nlistidx_t       idx;

  ret->valuetype = VALUE_NUM;
  idx = dfkeyBinarySearch (dancespeeddfkeys, DANCE_SPEED_DFKEY_COUNT, keydata);
  ret->u.num = dancespeeddfkeys [idx].itemkey;
}

