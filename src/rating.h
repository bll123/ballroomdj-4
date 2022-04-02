#ifndef INC_RATING_H
#define INC_RATING_H

#include "datafile.h"
#include "ilist.h"

typedef enum {
  RATING_RATING,
  RATING_WEIGHT,
  RATING_KEY_MAX
} ratingkey_t;

typedef struct {
  datafile_t        *df;
  ilist_t           *rating;
  int               maxWidth;
} rating_t;

#define RATING_UNRATED_IDX 0

rating_t    *ratingAlloc (char *);
void        ratingFree (rating_t *rating);
ssize_t     ratingGetCount (rating_t *rating);
int         ratingGetMaxWidth (rating_t *rating);
char        * ratingGetRating (rating_t *rating, ilistidx_t idx);
ssize_t     ratingGetWeight (rating_t *rating, ilistidx_t idx);
void        ratingStartIterator (rating_t *rating, ilistidx_t *iteridx);
ilistidx_t  ratingIterate (rating_t *rating, ilistidx_t *iteridx);
void        ratingConv (datafileconv_t *conv);

#endif /* INC_RATING_H */
