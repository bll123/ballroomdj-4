/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UILEVEL_H
#define INC_UILEVEL_H

#include "ui.h"

typedef struct uilevel uilevel_t;

uilevel_t * uilevelSpinboxCreate (UIWidget *boxp, bool allflag);
void uilevelFree (uilevel_t *uilevel);
int uilevelGetValue (uilevel_t *uilevel);
void uilevelSetValue (uilevel_t *uilevel, int value);
void uilevelDisable (uilevel_t *uilevel);
void uilevelEnable (uilevel_t *uilevel);
void uilevelSizeGroupAdd (uilevel_t *uilevel, UIWidget *sg);

#endif /* INC_UILEVEL_H */
