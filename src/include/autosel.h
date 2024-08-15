/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_AUTOSEL_H
#define INC_AUTOSEL_H

#include "datafile.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  AUTOSEL_BEG_COUNT,
  AUTOSEL_BEG_FAST,
  AUTOSEL_FAST_BOTH,
  AUTOSEL_FAST_PRIOR,
  AUTOSEL_HIST_DISTANCE,
  AUTOSEL_LEVEL_WEIGHT,
  AUTOSEL_LIMIT,
  AUTOSEL_LOW,
  AUTOSEL_PREV_TAGMATCH,
  AUTOSEL_PRIOR_EXP,
  AUTOSEL_RATING_WEIGHT,
  AUTOSEL_TAGMATCH,
  AUTOSEL_TAG_WEIGHT,
  AUTOSEL_TYPE_MATCH,
  /* windowed */
  AUTOSEL_WINDOWED_DIFF_A,
  AUTOSEL_WINDOWED_DIFF_B,
  AUTOSEL_WINDOWED_DIFF_C,
  AUTOSEL_KEY_MAX
} autoselkey_t;

typedef struct autosel autosel_t;

autosel_t     *autoselAlloc (void);
void          autoselFree (autosel_t *);
double        autoselGetDouble (autosel_t *autosel, autoselkey_t idx);
ssize_t       autoselGetNum (autosel_t *autosel, autoselkey_t idx);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_AUTOSEL_H */
