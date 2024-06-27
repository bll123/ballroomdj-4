/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UILEVEL_H
#define INC_UILEVEL_H

#include "callback.h"
#include "uiwcont.h"

typedef struct uilevel uilevel_t;

uilevel_t * uilevelSpinboxCreate (uiwcont_t *boxp, bool allflag);
void uilevelFree (uilevel_t *uilevel);
int uilevelGetValue (uilevel_t *uilevel);
void uilevelSetValue (uilevel_t *uilevel, int value);
void uilevelSetState (uilevel_t *uilevel, int state);
void uilevelSizeGroupAdd (uilevel_t *uilevel, uiwcont_t *sg);
void uilevelSetChangedCallback (uilevel_t *uilevel, callback_t *cb);

#endif /* INC_UILEVEL_H */
