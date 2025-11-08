/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "autosel.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "nodiscard.h"
#include "pathbld.h"

typedef struct autosel {
  datafile_t      *df;
  nlist_t         *autosel;
} autosel_t;

static datafilekey_t autoseldfkeys [] = {
  { "BEGINCOUNT",     AUTOSEL_BEG_COUNT,        VALUE_NUM,    NULL, DF_NORM },
  { "BEGINFAST",      AUTOSEL_BEG_FAST,         VALUE_DOUBLE, NULL, DF_NORM },
  { "BOTHFAST",       AUTOSEL_FAST_BOTH,        VALUE_DOUBLE, NULL, DF_NO_WRITE },
  { "COUNTLOW",       AUTOSEL_LOW,              VALUE_DOUBLE, NULL, DF_NORM },
  { "FASTBOTH",       AUTOSEL_FAST_BOTH,        VALUE_DOUBLE, NULL, DF_NORM },
  { "FASTPRIOR",      AUTOSEL_FAST_PRIOR,       VALUE_DOUBLE, NULL, DF_NORM },
  { "HISTDISTANCE",   AUTOSEL_HIST_DISTANCE,    VALUE_NUM,    NULL, DF_NORM },
  { "LEVELWEIGHT",    AUTOSEL_LEVEL_WEIGHT,     VALUE_DOUBLE, NULL, DF_NORM },
  { "LIMIT",          AUTOSEL_LIMIT,            VALUE_DOUBLE, NULL, DF_NORM },
  { "PREVTAGMATCH",   AUTOSEL_PREV_TAGMATCH,    VALUE_DOUBLE, NULL, DF_NORM },
  { "PRIOREXP",       AUTOSEL_PRIOR_EXP,        VALUE_DOUBLE, NULL, DF_NORM },
  { "RATINGWEIGHT",   AUTOSEL_RATING_WEIGHT,    VALUE_DOUBLE, NULL, DF_NORM },
  { "TAGMATCH",       AUTOSEL_TAGMATCH,         VALUE_DOUBLE, NULL, DF_NORM },
  { "TAGMATCHEXP",    AUTOSEL_PRIOR_EXP,        VALUE_DOUBLE, NULL, DF_NO_WRITE },
  { "TAGWEIGHT",      AUTOSEL_TAG_WEIGHT,       VALUE_DOUBLE, NULL, DF_NORM, },
  { "TYPEMATCH",      AUTOSEL_TYPE_MATCH,       VALUE_DOUBLE, NULL, DF_NORM },
  { "WINDIFFA",       AUTOSEL_WINDOWED_DIFF_A,  VALUE_DOUBLE, NULL, DF_NORM },
  { "WINDIFFB",       AUTOSEL_WINDOWED_DIFF_B,  VALUE_DOUBLE, NULL, DF_NORM },
  { "WINDIFFC",       AUTOSEL_WINDOWED_DIFF_C,  VALUE_DOUBLE, NULL, DF_NORM },
};
enum {
  autoseldfcount = sizeof (autoseldfkeys) / sizeof (datafilekey_t),
};

NODISCARD
autosel_t *
autoselAlloc (void)
{
  autosel_t   *autosel;
  char        fname [MAXPATHLEN];

  pathbldMakePath (fname, sizeof (fname), AUTOSEL_FN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: autosel: missing %s", fname);
    return NULL;
  }

  autosel = mdmalloc (sizeof (autosel_t));

  autosel->df = datafileAllocParse ("autosel", DFTYPE_KEY_VAL, fname,
      autoseldfkeys, autoseldfcount, DF_NO_OFFSET, NULL);
  autosel->autosel = datafileGetList (autosel->df);
  return autosel;
}

void
autoselFree (autosel_t *autosel)
{
  if (autosel != NULL) {
    datafileFree (autosel->df);
    mdfree (autosel);
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
    return LIST_VALUE_INVALID;
  }

  return nlistGetNum (autosel->autosel, idx);
}
