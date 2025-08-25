/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uirating uirating_t;

enum {
  UIRATING_NORM = false,
  UIRATING_ALL = true,
};

uirating_t * uiratingSpinboxCreate (uiwcont_t *boxp, bool allflag);
void uiratingFree (uirating_t *uirating);
int uiratingGetValue (uirating_t *uirating);
void uiratingSetValue (uirating_t *uirating, int value);
void uiratingSetState (uirating_t *uirating, int state);
void uiratingSizeGroupAdd (uirating_t *uirating, uiwcont_t *sg);
void uiratingSetChangedCallback (uirating_t *uirating, callback_t *cb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

