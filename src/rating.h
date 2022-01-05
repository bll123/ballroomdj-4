#ifndef INC_RATING_H
#define INC_RATING_H

#include "datafile.h"

typedef enum {
  RATING_RATING,
  RATING_WEIGHT,
  RATING_KEY_MAX
} ratingkey_t;

datafile_t *  ratingAlloc (char *);
void          ratingFree (datafile_t *);
list_t *      ratingGetList (datafile_t *df);
void          ratingConv (char *keydata, datafileret_t *ret);

#endif /* INC_RATING_H */
