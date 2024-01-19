/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_RATING_H
#define INC_RATING_H

#include "datafile.h"
#include "ilist.h"

typedef enum {
  RATING_RATING,
  RATING_WEIGHT,
  RATING_KEY_MAX
} ratingkey_t;

typedef struct rating rating_t;

enum {
  RATING_UNRATED_IDX = 0,
};

rating_t    *ratingAlloc (void);
void        ratingFree (rating_t *rating);
ssize_t     ratingGetCount (rating_t *rating);
int         ratingGetMaxWidth (rating_t *rating);
const char  * ratingGetRating (rating_t *rating, ilistidx_t idx);
ssize_t     ratingGetWeight (rating_t *rating, ilistidx_t idx);
void        ratingStartIterator (rating_t *rating, ilistidx_t *iteridx);
ilistidx_t  ratingIterate (rating_t *rating, ilistidx_t *iteridx);
void        ratingConv (datafileconv_t *conv);
void        ratingSave (rating_t *rating, ilist_t *list);

#endif /* INC_RATING_H */
