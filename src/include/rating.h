/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "datafile.h"
#include "ilist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  RATING_RATING,
  RATING_WEIGHT,
  RATING_KEY_MAX
} ratingkey_t;

typedef struct rating rating_t;

enum {
  RATING_UNRATED_IDX = 0,
};

NODISCARD rating_t    *ratingAlloc (void);
void        ratingFree (rating_t *ratings);
ilistidx_t  ratingGetCount (rating_t *ratings);
int         ratingGetMaxWidth (rating_t *ratings);
const char  * ratingGetRating (rating_t *ratings, ilistidx_t idx);
int         ratingGetWeight (rating_t *ratings, ilistidx_t idx);
void        ratingSetRating (rating_t *ratings, ilistidx_t ikey, const char *val);
void        ratingSetWeight (rating_t *ratings, ilistidx_t ikey, int val);
void        ratingDeleteLast (rating_t *ratings);
void        ratingStartIterator (rating_t *ratings, ilistidx_t *iteridx);
ilistidx_t  ratingIterate (rating_t *ratings, ilistidx_t *iteridx);
void        ratingConv (datafileconv_t *conv);
void        ratingSave (rating_t *ratings, ilist_t *list);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

