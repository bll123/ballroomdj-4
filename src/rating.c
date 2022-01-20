#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "ilist.h"
#include "rating.h"
#include "slist.h"

  /* must be sorted in ascii order */
static datafilekey_t ratingdfkeys[] = {
  { "RATING", RATING_RATING, VALUE_DATA, NULL },
  { "WEIGHT", RATING_WEIGHT, VALUE_NUM, NULL },
};
#define RATING_DFKEY_COUNT (sizeof (ratingdfkeys) / sizeof (datafilekey_t))

rating_t *
ratingAlloc (char *fname)
{
  rating_t        *rating;

  if (! fileopExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: rating: missing %s", fname);
    return NULL;
  }

  rating = malloc (sizeof (rating_t));
  assert (rating != NULL);

  rating->df = datafileAllocParse ("rating", DFTYPE_INDIRECT, fname,
      ratingdfkeys, RATING_DFKEY_COUNT, RATING_RATING);
  rating->rating = datafileGetList (rating->df);
  ilistDumpInfo (rating->rating);
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

ssize_t
ratingGetWeight (rating_t *rating, ilistidx_t ikey)
{
  return ilistGetNum (rating->rating, ikey, RATING_WEIGHT);
}

void
ratingConv (char *keydata, datafileret_t *ret)
{
  rating_t    *rating;
  slist_t      *lookup;

  ret->valuetype = VALUE_NUM;
  rating = bdjvarsdf [BDJVDF_RATINGS];
  lookup = datafileGetLookup (rating->df);
  ret->u.num = slistGetNum (lookup, keydata);
}
