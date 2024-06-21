/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "ilist.h"
#include "istring.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "rating.h"
#include "slist.h"

typedef struct rating {
  datafile_t        *df;
  ilist_t           *rating;
  slist_t           *ratingList;
  char              *path;
  int               maxWidth;
} rating_t;

/* must be sorted in ascii order */
static datafilekey_t ratingdfkeys [RATING_KEY_MAX] = {
  { "RATING", RATING_RATING, VALUE_STR, NULL, DF_NORM },
  { "WEIGHT", RATING_WEIGHT, VALUE_NUM, NULL, DF_NORM },
};

rating_t *
ratingAlloc (void)
{
  rating_t        *ratings;
  ilistidx_t      key;
  ilistidx_t      iteridx;
  char            fname [MAXPATHLEN];

  pathbldMakePath (fname, sizeof (fname), "ratings",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: rating: missing %s", fname);
    return NULL;
  }

  ratings = mdmalloc (sizeof (rating_t));

  ratings->path = mdstrdup (fname);
  ratings->df = datafileAllocParse ("rating", DFTYPE_INDIRECT, fname,
      ratingdfkeys, RATING_KEY_MAX, DF_NO_OFFSET, NULL);
  ratings->rating = datafileGetList (ratings->df);
  ilistDumpInfo (ratings->rating);

  ratings->ratingList = slistAlloc ("rating-disp", LIST_UNORDERED, NULL);
  slistSetSize (ratings->ratingList, ilistGetCount (ratings->rating));

  ratings->maxWidth = ilistGetMaxValueWidth (ratings->rating, RATING_RATING);

  ilistStartIterator (ratings->rating, &iteridx);
  while ((key = ilistIterateKey (ratings->rating, &iteridx)) >= 0) {
    const char  *val;

    val = ilistGetStr (ratings->rating, key, RATING_RATING);
    slistSetNum (ratings->ratingList, val, key);
  }
  slistSort (ratings->ratingList);

  return ratings;
}

void
ratingFree (rating_t *ratings)
{
  if (ratings == NULL) {
    return;
  }

  dataFree (ratings->path);
  datafileFree (ratings->df);
  slistFree (ratings->ratingList);
  mdfree (ratings);
}

ssize_t
ratingGetCount (rating_t *ratings)
{
  if (ratings == NULL) {
    return 0;
  }

  return ilistGetCount (ratings->rating);
}

int
ratingGetMaxWidth (rating_t *ratings)
{
  if (ratings == NULL) {
    return 0;
  }

  return ratings->maxWidth;
}

const char *
ratingGetRating (rating_t *ratings, ilistidx_t ikey)
{
  if (ratings == NULL) {
    return NULL;
  }

  return ilistGetStr (ratings->rating, ikey, RATING_RATING);
}

ssize_t
ratingGetWeight (rating_t *ratings, ilistidx_t ikey)
{
  if (ratings == NULL) {
    return LIST_VALUE_INVALID;
  }

  return ilistGetNum (ratings->rating, ikey, RATING_WEIGHT);
}

void
ratingSetRating (rating_t *ratings, ilistidx_t ikey, const char *val)
{
  if (ratings == NULL) {
    return;
  }

  ilistSetStr (ratings->rating, ikey, RATING_RATING, val);
}

void
ratingSetWeight (rating_t *ratings, ilistidx_t ikey, int val)
{
  if (ratings == NULL) {
    return;
  }

  ilistSetNum (ratings->rating, ikey, RATING_WEIGHT, val);
}

void
ratingDelete (rating_t *ratings, ilistidx_t ikey)
{
  if (ratings == NULL) {
    return;
  }

  ilistDelete (ratings->rating, ikey);
  ilistRenumber (ratings->rating);
}

void
ratingStartIterator (rating_t *ratings, ilistidx_t *iteridx)
{
  if (ratings == NULL) {
    return;
  }

  ilistStartIterator (ratings->rating, iteridx);
}

ilistidx_t
ratingIterate (rating_t *ratings, ilistidx_t *iteridx)
{
  if (ratings == NULL) {
    return LIST_VALUE_INVALID;
  }

  return ilistIterateKey (ratings->rating, iteridx);
}

void
ratingConv (datafileconv_t *conv)
{
  rating_t    *ratings;
  ssize_t     num;

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);

  if (conv->invt == VALUE_STR) {
    num = slistGetNum (ratings->ratingList, conv->str);
    if (num == LIST_VALUE_INVALID) {
      /* unknown ratings are dumped into the unrated bucket */
      num = RATING_UNRATED_IDX;
    }
    conv->outvt = VALUE_NUM;
    conv->num = num;
  } else if (conv->invt == VALUE_NUM) {
    num = conv->num;
    conv->outvt = VALUE_STR;
    conv->str = ilistGetStr (ratings->rating, num, RATING_RATING);
  }
}

void
ratingSave (rating_t *ratings, ilist_t *list)
{
  datafileSave (ratings->df, NULL, list, DF_NO_OFFSET,
      datafileDistVersion (ratings->df));
}
