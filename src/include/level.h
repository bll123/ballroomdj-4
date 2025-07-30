/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_LEVEL_H
#define INC_LEVEL_H

#include "datafile.h"
#include "ilist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  LEVEL_LEVEL,
  LEVEL_DEFAULT_FLAG,
  LEVEL_WEIGHT,
  LEVEL_KEY_MAX
} levelkey_t;

typedef struct level level_t;

[[nodiscard]] level_t     *levelAlloc (void);
void        levelFree (level_t *);
ilistidx_t  levelGetCount (level_t *levels);
int         levelGetMaxWidth (level_t *levels);

const char  * levelGetLevel (level_t *levels, ilistidx_t idx);
int levelGetWeight (level_t *levels, ilistidx_t idx);
int levelGetDefault (level_t *levels, ilistidx_t idx);
char        * levelGetDefaultName (level_t *levels);

void levelSetLevel (level_t *levels, ilistidx_t idx, const char *leveldisp);
void levelSetWeight (level_t *levels, ilistidx_t idx, int weight);
void levelSetDefault (level_t *levels, ilistidx_t idx);

void levelDeleteLast (level_t *levels);
ilistidx_t levelGetDefaultKey (level_t *levels);

void        levelStartIterator (level_t *levels, ilistidx_t *iteridx);
ilistidx_t  levelIterate (level_t *levels, ilistidx_t *iteridx);
void        levelConv (datafileconv_t *conv);
void        levelSave (level_t *levels, ilist_t *list);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_LEVEL_H */
