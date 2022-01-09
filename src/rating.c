#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "rating.h"
#include "datafile.h"
#include "fileop.h"
#include "bdjvarsdf.h"
#include "log.h"

  /* must be sorted in ascii order */
static datafilekey_t ratingdfkeys[] = {
  { "rating", RATING_RATING, VALUE_DATA, NULL },
  { "weight", RATING_WEIGHT, VALUE_LONG, NULL },
};
#define RATING_DFKEY_COUNT (sizeof (ratingdfkeys) / sizeof (datafilekey_t))

rating_t *
ratingAlloc (char *fname)
{
  rating_t        *rating;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: rating: missing %s\n", fname);
    return NULL;
  }

  rating = malloc (sizeof (rating_t));
  assert (rating != NULL);

  rating->df = datafileAllocParse ("rating", DFTYPE_KEY_LONG, fname,
      ratingdfkeys, RATING_DFKEY_COUNT, RATING_RATING);
  llistDumpInfo (datafileGetList (rating->df));
  return rating;
}

void
ratingFree (rating_t *rating)
{
  if (rating != NULL) {
    if (rating->df != NULL) {
      datafileFree (rating->df);
    }
    free (rating);
  }
}

void
ratingConv (char *keydata, datafileret_t *ret)
{
  rating_t    *rating;
  list_t      *lookup;

  ret->valuetype = VALUE_LONG;
  rating = bdjvarsdf [BDJVDF_RATINGS];
  lookup = datafileGetLookup (rating->df);
  ret->u.l = listGetLong (lookup, keydata);
}

