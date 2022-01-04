#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rating.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"

  /* must be sorted in ascii order */
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
  df = datafileAllocParse ("rating", DFTYPE_KEY_LONG, fname,
      ratingdfkeys, RATING_DFKEY_COUNT, RATING_RATING);
  return df;
}

void
ratingFree (datafile_t *df)
{
  datafileFree (df);
}

list_t *
ratingGetList (datafile_t *df)
{
  return df->data;
}

void
ratingConv (char *keydata, datafileret_t *ret)
{
  list_t      *lookup;

  ret->valuetype = VALUE_LONG;
  lookup = datafileGetLookup (bdjvarsdf [BDJVDF_RATINGS]);
  ret->u.l = listGetLong (lookup, keydata);
}

