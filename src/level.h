#ifndef INC_LEVEL_H
#define INC_LEVEL_H

#include "datafile.h"
#include "ilist.h"

typedef enum {
  LEVEL_LEVEL,
  LEVEL_DEFAULT_FLAG,
  LEVEL_WEIGHT,
  LEVEL_KEY_MAX
} levelkey_t;

typedef struct {
  datafile_t        *df;
  ilist_t           *level;
} level_t;

level_t     *levelAlloc (char *);
void        levelFree (level_t *);
ssize_t     levelGetWeight (level_t *level, listidx_t idx);
void        levelConv (char *keydata, datafileret_t *ret);

#endif /* INC_LEVEL_H */
