/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISTATUS_H
#define INC_UISTATUS_H

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uistatus uistatus_t;

uistatus_t * uistatusSpinboxCreate (uiwcont_t *boxp, bool allflag);
void uistatusFree (uistatus_t *uistatus);
int uistatusGetValue (uistatus_t *uistatus);
void uistatusSetValue (uistatus_t *uistatus, int value);
void uistatusSetState (uistatus_t *uistatus, int state);
void uistatusSizeGroupAdd (uistatus_t *uistatus, uiwcont_t *sg);
void uistatusSetChangedCallback (uistatus_t *uistatus, callback_t *cb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UISTATUS_H */
