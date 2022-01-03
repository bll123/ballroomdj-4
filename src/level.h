#ifndef INC_LEVEL_H
#define INC_LEVEL_H

#include "datafile.h"

typedef enum {
  LEVEL_LABEL,
  LEVEL_DEFAULT_FLAG,
  LEVEL_WEIGHT,
  LEVEL_KEY_MAX
} levelkey_t;

datafile_t *  levelAlloc (char *);
void          levelFree (datafile_t *);
void          levelConv (char *keydata, datafileret_t *ret);

#endif /* INC_LEVEL_H */
