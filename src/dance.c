#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dance.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"
#include "dnctypes.h"

static void danceConvSpeed (char *keydata, datafileret_t *ret);

  /* must be sorted in ascii order */
static datafilekey_t dancedfkeys[] = {
  { "ANNOUNCE",   DANCE_ANNOUNCE, VALUE_DATA, NULL },
  { "COUNT",      DANCE_COUNT, VALUE_LONG, NULL },
  { "DANCE",      DANCE_DANCE, VALUE_DATA, NULL },
  { "HIGHBPM",    DANCE_HIGH_BPM, VALUE_LONG, NULL },
  { "LOWBPM",     DANCE_LOW_BPM, VALUE_LONG, NULL },
  { "SELECT",     DANCE_SELECT, VALUE_LONG, NULL },
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

datafile_t *
danceAlloc (char *fname)
{
  datafile_t        *df;

  if (! fileopExists (fname)) {
    return NULL;
  }

  df = datafileAllocParse ("dance", DFTYPE_KEY_LONG, fname,
      dancedfkeys, DANCE_DFKEY_COUNT, DANCE_DANCE);
  return df;
}

void
danceFree (datafile_t *df)
{
  if (df != NULL) {
    datafileFree (df);
  }
}

/* internal routines */

static void
danceConvSpeed (char *keydata, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  long idx = dfkeyBinarySearch (dancespeeddfkeys, DANCE_SPEED_DFKEY_COUNT, keydata);
  ret->u.l = dancespeeddfkeys [idx].itemkey;
}

