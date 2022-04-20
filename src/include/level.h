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
  datafile_t  *df;
  ilist_t     *level;
  int         maxWidth;
  char        *path;
} level_t;

level_t     *levelAlloc (void);
void        levelFree (level_t *);
ssize_t     levelGetCount (level_t *level);
int         levelGetMaxWidth (level_t *level);
char        * levelGetLevel (level_t *level, ilistidx_t idx);
ssize_t     levelGetWeight (level_t *level, ilistidx_t idx);
ssize_t     levelGetDefault (level_t *level, ilistidx_t idx);
ssize_t     levelGetMax (level_t *level);
void        levelStartIterator (level_t *level, ilistidx_t *iteridx);
ilistidx_t  levelIterate (level_t *level, ilistidx_t *iteridx);
void        levelConv (datafileconv_t *conv);
void        levelSave (level_t *level, ilist_t *list);

#endif /* INC_LEVEL_H */
