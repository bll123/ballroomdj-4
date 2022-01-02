#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rating.h"
#include "datafile.h"
#include "fileop.h"

static datafilekey_t ratingdfkeys[] = {
  { "rating", RATING_RATING, VALUE_DATA, NULL },
  { "weight", RATING_WEIGHT, VALUE_LONG, NULL },
};
#define RATING_DFKEY_COUNT (sizeof (ratingdfkeys) / sizeof (datafilekey_t))

datafile_t *
ratingAlloc (char *fname)
{
  datafile_t    *df;

  if (! fileopExists (fname)) {
    return NULL;
  }
  df = datafileAlloc (ratingdfkeys, RATING_DFKEY_COUNT, fname, DFTYPE_KEY_LONG);
  return df;
}

void
ratingFree (datafile_t *df)
{
  datafileFree (df);
}
