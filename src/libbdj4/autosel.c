/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "autosel.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "nlist.h"
#include "pathbld.h"

typedef struct autosel {
  datafile_t      *df;
  nlist_t         *autosel;
} autosel_t;

static datafilekey_t autoseldfkeys [AUTOSEL_KEY_MAX] = {
  { "BEGINCOUNT",     AUTOSEL_BEG_COUNT,        VALUE_NUM,    NULL, -1 },
  { "BEGINFAST",      AUTOSEL_BEG_FAST,         VALUE_DOUBLE, NULL, -1 },
  { "BOTHFAST",       AUTOSEL_BOTHFAST,         VALUE_DOUBLE, NULL, -1 },
  { "COUNTLOW",       AUTOSEL_LOW,              VALUE_DOUBLE, NULL, -1 },
  { "EXPECTEDHIGH",   AUTOSEL_EXPECTED_HIGH,    VALUE_DOUBLE, NULL, -1 },
  { "EXPECTEDMULT",   AUTOSEL_EXPECTED_MULT,    VALUE_DOUBLE, NULL, -1 },
  { "EXPECTEDVAR",    AUTOSEL_EXPECTED_VAR,     VALUE_DOUBLE, NULL, -1 },
  { "HISTDISTANCE",   AUTOSEL_HIST_DISTANCE,    VALUE_NUM,    NULL, -1 },
  { "LEVELWEIGHT",    AUTOSEL_LEVEL_WEIGHT,     VALUE_DOUBLE, NULL, -1 },
  { "LIMIT",          AUTOSEL_LIMIT,            VALUE_DOUBLE, NULL, -1 },
  { "PREVTAGMATCH",   AUTOSEL_PREV_TAGMATCH,    VALUE_DOUBLE, NULL, -1 },
  { "PRIORVAR",       AUTOSEL_PRIOR_VAR,        VALUE_DOUBLE, NULL, -1 },
  { "RATINGWEIGHT",   AUTOSEL_RATING_WEIGHT,    VALUE_DOUBLE, NULL, -1 },
  { "TAGMATCH",       AUTOSEL_TAGMATCH,         VALUE_DOUBLE, NULL, -1 },
  { "TAGMATCHEXP",    AUTOSEL_TAGMATCH_EXP,     VALUE_DOUBLE, NULL, -1 },
  { "TYPEMATCH",      AUTOSEL_TYPE_MATCH,       VALUE_DOUBLE, NULL, -1 },
};

autosel_t *
autoselAlloc (void)
{
  autosel_t   *autosel;
  char        fname [MAXPATHLEN];

  pathbldMakePath (fname, sizeof (fname), "autoselection",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: autosel: missing %s", fname);
    return NULL;
  }

  autosel = malloc (sizeof (autosel_t));
  assert (autosel != NULL);

  autosel->df = datafileAllocParse ("autosel", DFTYPE_KEY_VAL, fname,
      autoseldfkeys, AUTOSEL_KEY_MAX);
  autosel->autosel = datafileGetList (autosel->df);
  return autosel;
}

void
autoselFree (autosel_t *autosel)
{
  if (autosel != NULL) {
    datafileFree (autosel->df);
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
