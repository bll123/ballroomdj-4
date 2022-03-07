#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "ilist.h"
#include "rating.h"
#include "slist.h"

  /* must be sorted in ascii order */
static datafilekey_t ratingdfkeys [RATING_KEY_MAX] = {
  { "RATING", RATING_RATING, VALUE_STR, NULL, -1 },
  { "WEIGHT", RATING_WEIGHT, VALUE_NUM, NULL, -1 },
};

rating_t *
ratingAlloc (char *fname)
{
  rating_t        *rating;
  ilistidx_t      key;
  ilistidx_t      iteridx;

  if (! fileopFileExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: rating: missing %s", fname);
    return NULL;
  }

  rating = malloc (sizeof (rating_t));
  assert (rating != NULL);

  rating->df = datafileAllocParse ("rating", DFTYPE_INDIRECT, fname,
      ratingdfkeys, RATING_KEY_MAX, RATING_RATING);
  rating->rating = datafileGetList (rating->df);
  ilistDumpInfo (rating->rating);

  rating->maxWidth = 0;
  ilistStartIterator (rating->rating, &iteridx);
  while ((key = ilistIterateKey (rating->rating, &iteridx)) >= 0) {
    char    *val;
    int     len;

    val = ilistGetStr (rating->rating, key, RATING_RATING);
    len = istrlen (val);
    if (len > rating->maxWidth) {
      rating->maxWidth = len;
    }
  }

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
ratingGetCount (rating_t *rating)
{
  return ilistGetCount (rating->rating);
}

int
ratingGetMaxWidth (rating_t *rating)
{
  return rating->maxWidth;
}

char *
ratingGetRating (rating_t *rating, ilistidx_t ikey)
{
  return ilistGetStr (rating->rating, ikey, RATING_RATING);
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
  rating = bdjvarsdfGet (BDJVDF_RATINGS);
  lookup = datafileGetLookup (rating->df);
  ret->u.num = slistGetNum (lookup, keydata);
  if (ret->u.num == LIST_VALUE_INVALID) {
    /* unknown ratings are dumped into the unrated bucket */
    ret->u.num = RATING_UNRATED_IDX;
  }
}
