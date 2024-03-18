/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISWITCH_H
#define INC_UISWITCH_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "callback.h"
#include "uiwcont.h"

uiwcont_t *uiCreateSwitch (int value);
void uiSwitchFree (uiwcont_t *uiwidget);
void uiSwitchSetValue (uiwcont_t *uiwidget, int value);
int uiSwitchGetValue (uiwcont_t *uiwidget);
void uiSwitchSetCallback (uiwcont_t *uiwidget, callback_t *uicb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UISWITCH_H */
