#ifndef INC_AUTOSEL_H
#define INC_AUTOSEL_H

#include "datafile.h"
#include "nlist.h"

typedef enum {
  AUTOSEL_BEG_COUNT,
  AUTOSEL_BEG_FAST,
  AUTOSEL_BOTHFAST,
  AUTOSEL_EXPECTED_HIGH,
  AUTOSEL_EXPECTED_MULT,
  AUTOSEL_EXPECTED_VAR,
  AUTOSEL_HIST_DISTANCE,
  AUTOSEL_LEVEL_WEIGHT,
  AUTOSEL_LIMIT,
  AUTOSEL_LOW,
  AUTOSEL_PREV_TAGMATCH,
  AUTOSEL_PRIOR_VAR,
  AUTOSEL_RATING_WEIGHT,
  AUTOSEL_TAGMATCH,
  AUTOSEL_TAGMATCH_EXP,
  AUTOSEL_TYPE_MATCH,
  AUTOSEL_KEY_MAX
} autoselkey_t;

typedef struct autosel autosel_t;

autosel_t     *autoselAlloc (void);
void          autoselFree (autosel_t *);
double        autoselGetDouble (autosel_t *autosel, autoselkey_t idx);
ssize_t       autoselGetNum (autosel_t *autosel, autoselkey_t idx);

#endif /* INC_AUTOSEL_H */
