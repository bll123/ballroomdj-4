/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateSwitch (int value);
void uiSwitchFree (uiwcont_t *uiwidget);
void uiSwitchSetValue (uiwcont_t *uiwidget, int value);
int uiSwitchGetValue (uiwcont_t *uiwidget);
void uiSwitchSetCallback (uiwcont_t *uiwidget, callback_t *uicb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

