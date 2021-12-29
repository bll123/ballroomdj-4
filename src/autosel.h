#ifndef INC_AUTOSEL_H
#define INC_AUTOSEL_H

#include "datafile.h"

typedef enum {
  AUTOSEL_BEG_COUNT,
  AUTOSEL_BEG_FAST,
  AUTOSEL_BOTHFAST,
  AUTOSEL_HIST_DISTANCE,
  AUTOSEL_LEVEL_WEIGHT,
  AUTOSEL_LIMIT,
  AUTOSEL_LOG_VALUE,
  AUTOSEL_LOW,
  AUTOSEL_RATING_WEIGHT,
  AUTOSEL_TAGMATCH,
  AUTOSEL_TYPE_MATCH,
  AUTOSEL_KEY_MAX
} autoselkey_t;

datafile_t *  autoselAlloc (char *);
void          autoselFree (datafile_t *);

#endif /* INC_AUTOSEL_H */
