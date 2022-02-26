#ifndef INC_RATING_H
#define INC_RATING_H

#include "datafile.h"
#include "nlist.h"

typedef enum {
  RATING_RATING,
  RATING_WEIGHT,
  RATING_KEY_MAX
} ratingkey_t;

typedef struct {
  datafile_t        *df;
  nlist_t           *rating;
  int               maxWidth;
} rating_t;

#define RATING_UNRATED_IDX 0

rating_t    *ratingAlloc (char *);
void        ratingFree (rating_t *rating);
ssize_t     ratingGetCount (rating_t *rating);
int         ratingGetMaxWidth (rating_t *rating);
char        * ratingGetRating (rating_t *rating, nlistidx_t idx);
ssize_t     ratingGetWeight (rating_t *rating, nlistidx_t idx);
void        ratingConv (char *keydata, datafileret_t *ret);

#endif /* INC_RATING_H */
