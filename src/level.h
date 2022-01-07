#ifndef INC_LEVEL_H
#define INC_LEVEL_H

#include "datafile.h"

typedef enum {
  LEVEL_LABEL,
  LEVEL_DEFAULT_FLAG,
  LEVEL_WEIGHT,
  LEVEL_KEY_MAX
} levelkey_t;

typedef struct {
  datafile_t        *df;
} level_t;

level_t     *levelAlloc (char *);
void        levelFree (level_t *);
void        levelConv (char *keydata, datafileret_t *ret);

#endif /* INC_LEVEL_H */
