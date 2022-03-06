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
#include "nlist.h"

static datafilekey_t autoseldfkeys [AUTOSEL_KEY_MAX] = {
  { "begincount",     AUTOSEL_BEG_COUNT,        VALUE_NUM,    NULL, -1 },
  { "beginfast",      AUTOSEL_BEG_FAST,         VALUE_DOUBLE, NULL, -1 },
  { "bothfast",       AUTOSEL_BOTHFAST,         VALUE_DOUBLE, NULL, -1 },
  { "histdistance",   AUTOSEL_HIST_DISTANCE,    VALUE_NUM,   NULL, -1 },
  { "levelweight",    AUTOSEL_LEVEL_WEIGHT,     VALUE_DOUBLE, NULL, -1 },
  { "limit",          AUTOSEL_LIMIT,            VALUE_DOUBLE, NULL, -1 },
  { "logvalue",       AUTOSEL_LOG_VALUE,        VALUE_DOUBLE, NULL, -1 },
  { "low",            AUTOSEL_LOW,              VALUE_DOUBLE, NULL, -1 },
  { "ratingweight",   AUTOSEL_RATING_WEIGHT,    VALUE_DOUBLE, NULL, -1 },
  { "tagadjust",      AUTOSEL_TAGADJUST,        VALUE_DOUBLE, NULL, -1 },
  { "tagmatch",       AUTOSEL_TAGMATCH,         VALUE_DOUBLE, NULL, -1 },
  { "typematch",      AUTOSEL_TYPE_MATCH,       VALUE_DOUBLE, NULL, -1 },
};

autosel_t *
autoselAlloc (char *fname)
{
  autosel_t     *autosel;

  if (! fileopFileExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: autosel: missing %s", fname);
    return NULL;
  }

  autosel = malloc (sizeof (autosel_t));
  assert (autosel != NULL);

  autosel->df = datafileAllocParse ("autosel", DFTYPE_KEY_VAL, fname,
      autoseldfkeys, AUTOSEL_KEY_MAX, DATAFILE_NO_LOOKUP);
  autosel->autosel = datafileGetList (autosel->df);
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

double
autoselGetDouble (autosel_t *autosel, autoselkey_t idx)
{
  if (autosel == NULL || autosel->autosel == NULL) {
    return LIST_DOUBLE_INVALID;
  }

  return nlistGetDouble (autosel->autosel, idx);
}

ssize_t
autoselGetNum (autosel_t *autosel, autoselkey_t idx)
{
  if (autosel == NULL || autosel->autosel == NULL) {
    return LIST_LOC_INVALID;
  }

  return nlistGetNum (autosel->autosel, idx);
}
