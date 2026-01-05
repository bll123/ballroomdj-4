/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uilevel uilevel_t;

uilevel_t * uilevelSpinboxCreate (uiwcont_t *boxp, bool allflag);
void uilevelFree (uilevel_t *uilevel);
int uilevelGetValue (uilevel_t *uilevel);
void uilevelSetValue (uilevel_t *uilevel, int value);
void uilevelSetState (uilevel_t *uilevel, int state);
void uilevelSizeGroupAdd (uilevel_t *uilevel, uiwcont_t *sg);
void uilevelSetChangedCallback (uilevel_t *uilevel, callback_t *cb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

