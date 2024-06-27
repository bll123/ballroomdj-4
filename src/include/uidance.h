/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIDANCE_H
#define INC_UIDANCE_H

#include "callback.h"
#include "ilist.h"
#include "uiwcont.h"

enum {
  UIDANCE_PACK_START,
  UIDANCE_PACK_END,
  UIDANCE_NONE,
  UIDANCE_ALL_DANCES,
  UIDANCE_EMPTY_DANCE,
};

typedef struct uidance uidance_t;

uidance_t * uidanceCreate (uiwcont_t *boxp, uiwcont_t *parentwin, int flags, const char *label, int where, int count);
uiwcont_t * uidanceGetButton (uidance_t *uidance);
void uidanceFree (uidance_t *uidance);
ilistidx_t uidanceGetKey (uidance_t *uidance);
void uidanceSetKey (uidance_t *uidance, ilistidx_t dkey);
void uidanceSetState (uidance_t *uidance, int state);
void uidanceSizeGroupAdd (uidance_t *uidance, uiwcont_t *sg);
void uidanceSetCallback (uidance_t *uidance, callback_t *cb);

#endif /* INC_UIDANCE_H */
