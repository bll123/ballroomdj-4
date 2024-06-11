/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIKEYS_H
#define INC_UIKEYS_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "callback.h"
#include "uiwcont.h"

uiwcont_t * uiKeyAlloc (void);
void    uiKeyFree (uiwcont_t *uiwidget);
uiwcont_t * uiKeyCreateEventBox (uiwcont_t *uiwidgetp);
void    uiKeySetKeyCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb);
void    uiKeySetButtonCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb);
int     uiKeyEvent (uiwcont_t *uiwidget);
bool    uiKeyIsPressEvent (uiwcont_t *uiwidget);
bool    uiKeyIsReleaseEvent (uiwcont_t *uiwidget);
bool    uiKeyIsMovementKey (uiwcont_t *uiwidget);
bool    uiKeyIsKey (uiwcont_t *uiwidget, unsigned char keyval);
bool    uiKeyIsEnterKey (uiwcont_t *uiwidget);
bool    uiKeyIsAudioPlayKey (uiwcont_t *uiwidget);
bool    uiKeyIsAudioPauseKey (uiwcont_t *uiwidget);
bool    uiKeyIsAudioNextKey (uiwcont_t *uiwidget);
bool    uiKeyIsAudioPrevKey (uiwcont_t *uiwidget);
bool    uiKeyIsUpKey (uiwcont_t *uiwidget);
bool    uiKeyIsDownKey (uiwcont_t *uiwidget);
bool    uiKeyIsPageUpDownKey (uiwcont_t *uiwidget);
bool    uiKeyIsNavKey (uiwcont_t *uiwidget);
bool    uiKeyIsPasteCutKey (uiwcont_t *uiwidget);
bool    uiKeyIsMaskedKey (uiwcont_t *uiwidget);
bool    uiKeyIsAltPressed (uiwcont_t *uiwidget);
bool    uiKeyIsControlPressed (uiwcont_t *uiwidget);
bool    uiKeyIsShiftPressed (uiwcont_t *uiwidget);
bool    uiKeyButtonPressed (uiwcont_t *uiwidget, uint button);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIKEYS_H */
