/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UILEVEL_H
#define INC_UILEVEL_H

#include "callback.h"
#include "ui.h"

typedef struct uilevel uilevel_t;

uilevel_t * uilevelSpinboxCreate (uiwidget_t *boxp, bool allflag);
void uilevelFree (uilevel_t *uilevel);
int uilevelGetValue (uilevel_t *uilevel);
void uilevelSetValue (uilevel_t *uilevel, int value);
void uilevelSetState (uilevel_t *uilevel, int state);
void uilevelSizeGroupAdd (uilevel_t *uilevel, uiwidget_t *sg);
void uilevelSetChangedCallback (uilevel_t *uilevel, callback_t *cb);

#endif /* INC_UILEVEL_H */
