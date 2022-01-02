#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dance.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"

static void danceConvSpeed (const char *data, datafileret_t *ret);
static void danceConvType (const char *data, datafileret_t *ret);

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
  { "TYPE",       DANCE_TYPE, VALUE_DATA, danceConvType },
};
#define DANCE_DFKEY_COUNT (sizeof (dancedfkeys) / sizeof (datafilekey_t))

datafile_t *
danceAlloc (char *fname)
{
  datafile_t    *df;

  if (! fileopExists (fname)) {
    return NULL;
  }
  df = datafileAllocParse ("dance", DFTYPE_KEY_LONG, fname, dancedfkeys, RATING_DFKEY_COUNT);
  return df;
}

void
danceFree (datafile_t *df)
{
  datafileFree (df);
}


/* internal routines */

static void
danceConvSpeed (const char *data, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  ret->u.l = 0;
}

static void
danceConvType (const char *data, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  ret->u.l = 0;
}

