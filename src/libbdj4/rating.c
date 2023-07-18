/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
  rating_t        *rating;
  ilistidx_t      key;
  ilistidx_t      iteridx;
  char            fname [MAXPATHLEN];

  pathbldMakePath (fname, sizeof (fname), "ratings",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: rating: missing %s", fname);
    return NULL;
  }

  rating = mdmalloc (sizeof (rating_t));

  rating->path = mdstrdup (fname);
  rating->df = datafileAllocParse ("rating", DFTYPE_INDIRECT, fname,
      ratingdfkeys, RATING_KEY_MAX, DF_NO_OFFSET, NULL);
  rating->rating = datafileGetList (rating->df);
  ilistDumpInfo (rating->rating);

  rating->maxWidth = 0;
  rating->ratingList = slistAlloc ("rating-disp", LIST_UNORDERED, NULL);
  slistSetSize (rating->ratingList, ilistGetCount (rating->rating));

  ilistStartIterator (rating->rating, &iteridx);
  while ((key = ilistIterateKey (rating->rating, &iteridx)) >= 0) {
    const char  *val;
    int         len;

    val = ilistGetStr (rating->rating, key, RATING_RATING);
    slistSetNum (rating->ratingList, val, key);
    len = istrlen (val);
    if (len > rating->maxWidth) {
      rating->maxWidth = len;
    }
  }
  slistSort (rating->ratingList);

  return rating;
}

void
ratingFree (rating_t *rating)
{
  if (rating != NULL) {
    dataFree (rating->path);
    datafileFree (rating->df);
    slistFree (rating->ratingList);
    mdfree (rating);
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

const char *
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
ratingStartIterator (rating_t *rating, ilistidx_t *iteridx)
{
  ilistStartIterator (rating->rating, iteridx);
}

ilistidx_t
ratingIterate (rating_t *rating, ilistidx_t *iteridx)
{
  return ilistIterateKey (rating->rating, iteridx);
}

void
ratingConv (datafileconv_t *conv)
{
  rating_t    *rating;
  ssize_t     num;

  rating = bdjvarsdfGet (BDJVDF_RATINGS);

  if (conv->invt == VALUE_STR) {
    num = slistGetNum (rating->ratingList, conv->str);
    if (num == LIST_VALUE_INVALID) {
      /* unknown ratings are dumped into the unrated bucket */
      num = RATING_UNRATED_IDX;
    }
    conv->outvt = VALUE_NUM;
    conv->num = num;
  } else if (conv->invt == VALUE_NUM) {
    num = conv->num;
    conv->outvt = VALUE_STR;
    conv->str = ilistGetStr (rating->rating, num, RATING_RATING);
  }
}

void
ratingSave (rating_t *rating, ilist_t *list)
{
  datafileSave (rating->df, NULL, list, DF_NO_OFFSET,
      datafileDistVersion (rating->df));
}
