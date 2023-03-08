/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISTATUS_H
#define INC_UISTATUS_H

#include "callback.h"
#include "ui.h"

typedef struct uistatus uistatus_t;

uistatus_t * uistatusSpinboxCreate (UIWidget *boxp, bool allflag);
void uistatusFree (uistatus_t *uistatus);
int uistatusGetValue (uistatus_t *uistatus);
void uistatusSetValue (uistatus_t *uistatus, int value);
void uistatusSetState (uistatus_t *uistatus, int state);
void uistatusSizeGroupAdd (uistatus_t *uistatus, UIWidget *sg);
void uistatusSetChangedCallback (uistatus_t *uistatus, callback_t *cb);

#endif /* INC_UISTATUS_H */
