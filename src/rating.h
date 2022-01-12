#ifndef INC_RATING_H
#define INC_RATING_H

#include "datafile.h"
#include "list.h"

typedef enum {
  RATING_RATING,
  RATING_WEIGHT,
  RATING_KEY_MAX
} ratingkey_t;

typedef struct {
  datafile_t        *df;
  list_t            *rating;
} rating_t;

rating_t    *ratingAlloc (char *);
void        ratingFree (rating_t *rating);
ssize_t     ratingGetWeight (rating_t *rating, listidx_t idx);
void        ratingConv (char *keydata, datafileret_t *ret);

#endif /* INC_RATING_H */
