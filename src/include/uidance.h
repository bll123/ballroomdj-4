/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIDANCE_H
#define INC_UIDANCE_H

#include "ui.h"

enum {
  UIDANCE_PACK_START,
  UIDANCE_PACK_END,
  UIDANCE_NONE,
  UIDANCE_ALL_DANCES,
  UIDANCE_EMPTY_DANCE,
};

typedef struct uidance uidance_t;

uidance_t * uidanceDropDownCreate (uiwcont_t *boxp, uiwcont_t *parentwin, int flags, const char *label, int where, int count);
uiwcont_t * uidanceGetButton (uidance_t *uidance);
void uidanceFree (uidance_t *uidance);
int uidanceGetValue (uidance_t *uidance);
void uidanceSetValue (uidance_t *uidance, int value);
void uidanceSetState (uidance_t *uidance, int state);
void uidanceSizeGroupAdd (uidance_t *uidance, uiwcont_t *sg);
void uidanceSetCallback (uidance_t *uidance, callback_t *cb);

#endif /* INC_UIDANCE_H */
