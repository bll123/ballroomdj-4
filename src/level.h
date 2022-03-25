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
  int               maxWidth;
} level_t;

level_t     *levelAlloc (char *);
void        levelFree (level_t *);
ssize_t     levelGetCount (level_t *level);
int         levelGetMaxWidth (level_t *level);
char        * levelGetLevel (level_t *level, ilistidx_t idx);
ssize_t     levelGetWeight (level_t *level, ilistidx_t idx);
ssize_t     levelGetMax (level_t *level);
void        levelConv (datafileconv_t *conv);

#endif /* INC_LEVEL_H */
