#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "dance.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"
#include "dnctypes.h"
#include "log.h"

static void danceConvSpeed (char *keydata, datafileret_t *ret);

  /* must be sorted in ascii order */
static datafilekey_t dancedfkeys[] = {
  { "ANNOUNCE",   DANCE_ANNOUNCE, VALUE_DATA, NULL },
  { "COUNT",      DANCE_COUNT, VALUE_NUM, NULL },
  { "DANCE",      DANCE_DANCE, VALUE_DATA, NULL },
  { "HIGHBPM",    DANCE_HIGH_BPM, VALUE_NUM, NULL },
  { "LOWBPM",     DANCE_LOW_BPM, VALUE_NUM, NULL },
  { "SELECT",     DANCE_SELECT, VALUE_NUM, NULL },
  { "SPEED",      DANCE_SPEED, VALUE_LIST, danceConvSpeed },
  { "TAGS",       DANCE_TAGS, VALUE_LIST, parseConvTextList },
  { "TIMESIG",    DANCE_TIMESIG, VALUE_DATA, NULL },
  { "TYPE",       DANCE_TYPE, VALUE_DATA, dnctypesConv },
};
#define DANCE_DFKEY_COUNT (sizeof (dancedfkeys) / sizeof (datafilekey_t))

static datafilekey_t dancespeeddfkeys[] = {
  { "fast",       DANCE_SPEED_FAST,   VALUE_DATA, NULL },
  { "normal",     DANCE_SPEED_NORMAL, VALUE_DATA, NULL },
  { "slow",       DANCE_SPEED_SLOW,   VALUE_DATA, NULL },
};
#define DANCE_SPEED_DFKEY_COUNT (sizeof (dancespeeddfkeys) / sizeof (datafilekey_t))

dance_t *
danceAlloc (char *fname)
{
  dance_t           *dance;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "dance: missing: %s", fname);
    return NULL;
  }

  dance = malloc (sizeof (dance_t));
  assert (dance != NULL);

  dance->df = datafileAllocParse ("dance", DFTYPE_INDIRECT, fname,
      dancedfkeys, DANCE_DFKEY_COUNT, DANCE_DANCE);
  return dance;
}

void
danceFree (dance_t *dance)
{
  if (dance != NULL) {
    if (dance->df != NULL) {
      datafileFree (dance->df);
    }
    free (dance);
  }
}

list_t *
danceGetLookup (void)
{
  dance_t *dance = bdjvarsdf [BDJVDF_DANCES];
  list_t  *lookup = datafileGetLookup (dance->df);
  return lookup;
}

void
danceConvDance (char *keydata, datafileret_t *ret)
{
  dance_t       *dance;
  list_t        *lookup;

  ret->valuetype = VALUE_NUM;
  ret->u.num = -1;
  dance = bdjvarsdf [BDJVDF_DANCES];
  lookup = datafileGetLookup (dance->df);
  if (lookup != NULL) {
    ret->u.num = listGetNum (lookup, keydata);
  }
}

/* internal routines */

static void
danceConvSpeed (char *keydata, datafileret_t *ret)
{
  ret->valuetype = VALUE_NUM;
  listidx_t idx = dfkeyBinarySearch (dancespeeddfkeys, DANCE_SPEED_DFKEY_COUNT, keydata);
  ret->u.num = dancespeeddfkeys [idx].itemkey;
}

