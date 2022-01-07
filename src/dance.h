#ifndef INC_DANCE_H
#define INC_DANCE_H

#include "datafile.h"

typedef enum {
  DANCE_ANNOUNCE,
  DANCE_COUNT,
  DANCE_DANCE,
  DANCE_HIGH_BPM,
  DANCE_LOW_BPM,
  DANCE_SELECT,
  DANCE_SPEED,
  DANCE_TAGS,
  DANCE_TIMESIG,
  DANCE_TYPE,
  DANCE_KEY_MAX,
} dancekey_t;

typedef enum {
  DANCE_SPEED_SLOW,
  DANCE_SPEED_NORMAL,
  DANCE_SPEED_FAST,
} dancespeed_t;

typedef struct {
  datafile_t      *df;
} dance_t;

dance_t       *danceAlloc (char *);
void          danceFree (dance_t *);
void          danceConvDance (char *keydata, datafileret_t *ret);

#endif /* INC_DANCE_H */
