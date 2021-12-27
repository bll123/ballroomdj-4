#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rating.h"
#include "datafile.h"

static datafilekey_t ratingdfkeys[] = {
  { RATING_RATING, "rating", VALUE_DATA },
  { RATING_WEIGHT, "weight", VALUE_LONG },
};
#define RATING_DFKEY_COUNT (sizeof (ratingdfkeys) / sizeof (datafilekey_t))

datafile_t *
ratingAlloc (char *fname)
{
  datafile_t    *df;

  df = datafileAlloc (ratingdfkeys, RATING_DFKEY_COUNT, fname);
  return df;
}

void
ratingFree (datafile_t *df)
{
  datafileFree (df);
}
