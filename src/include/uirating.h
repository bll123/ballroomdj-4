/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIRATING_H
#define INC_UIRATING_H

#include "callback.h"
#include "ui.h"

typedef struct uirating uirating_t;

uirating_t * uiratingSpinboxCreate (UIWidget *boxp, bool allflag);
void uiratingFree (uirating_t *uirating);
int uiratingGetValue (uirating_t *uirating);
void uiratingSetValue (uirating_t *uirating, int value);
void uiratingDisable (uirating_t *uirating);
void uiratingEnable (uirating_t *uirating);
void uiratingSizeGroupAdd (uirating_t *uirating, UIWidget *sg);
void uiratingSetChangedCallback (uirating_t *uirating, callback_t *cb);

#endif /* INC_UIRATING_H */
