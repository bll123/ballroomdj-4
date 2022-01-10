#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "autosel.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"

static datafilekey_t autoseldfkeys[] = {
  { "begincount",     AUTOSEL_BEG_COUNT,        VALUE_LONG,    NULL },
  { "beginfast",      AUTOSEL_BEG_FAST,         VALUE_DOUBLE, NULL },
  { "bothfast",       AUTOSEL_BOTHFAST,         VALUE_DOUBLE, NULL },
  { "histdistance",   AUTOSEL_HIST_DISTANCE,    VALUE_LONG,   NULL },
  { "levelweight",    AUTOSEL_LEVEL_WEIGHT,     VALUE_DOUBLE, NULL },
  { "limit",          AUTOSEL_LIMIT,            VALUE_DOUBLE, NULL },
  { "logvalue",       AUTOSEL_LOG_VALUE,        VALUE_DOUBLE, NULL },
  { "low",            AUTOSEL_LOW,              VALUE_DOUBLE, NULL },
  { "ratingweight",   AUTOSEL_RATING_WEIGHT,    VALUE_DOUBLE, NULL },
  { "tagadjust",      AUTOSEL_TAGADJUST,        VALUE_DOUBLE, NULL },
  { "tagmatch",       AUTOSEL_TAGMATCH,         VALUE_DOUBLE, NULL },
  { "typematch",      AUTOSEL_TYPE_MATCH,       VALUE_DOUBLE, NULL },
};
#define AUTOSEL_DFKEY_COUNT (sizeof (autoseldfkeys) / sizeof (datafilekey_t))

autosel_t *
autoselAlloc (char *fname)
{
  autosel_t     *autosel;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: autosel: missing %s", fname);
    return NULL;
  }

  autosel = malloc (sizeof (autosel_t));
  assert (autosel != NULL);

  autosel->df = datafileAllocParse ("autosel", DFTYPE_KEY_VAL, fname,
      autoseldfkeys, AUTOSEL_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  return autosel;
}

void
autoselFree (autosel_t *autosel)
{
  if (autosel != NULL) {
    if (autosel->df != NULL) {
      datafileFree (autosel->df);
    }
    free (autosel);
  }
}
