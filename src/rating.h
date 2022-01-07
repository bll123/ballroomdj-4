#ifndef INC_RATING_H
#define INC_RATING_H

#include "datafile.h"

typedef enum {
  RATING_RATING,
  RATING_WEIGHT,
  RATING_KEY_MAX
} ratingkey_t;

typedef struct {
  datafile_t        *df;
} rating_t;

rating_t    *ratingAlloc (char *);
void        ratingFree (rating_t *rating);
void        ratingConv (char *keydata, datafileret_t *ret);

#endif /* INC_RATING_H */
