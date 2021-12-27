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

#endif /* INC_RATING_H */
