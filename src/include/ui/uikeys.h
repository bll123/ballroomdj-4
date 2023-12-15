/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIKEYS_H
#define INC_UIKEYS_H

#include "callback.h"
#include "uiwcont.h"

uiwcont_t * uiKeyAlloc (void);
void    uiKeyFree (uiwcont_t *uiwidget);
void    uiKeySetKeyCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb);
int     uiKeyEvent (uiwcont_t *uiwidget);
bool    uiKeyIsPressEvent (uiwcont_t *uiwidget);
bool    uiKeyIsReleaseEvent (uiwcont_t *uiwidget);
bool    uiKeyIsMovementKey (uiwcont_t *uiwidget);
bool    uiKeyIsKey (uiwcont_t *uiwidget, unsigned char keyval);
bool    uiKeyIsAudioPlayKey (uiwcont_t *uiwidget);
bool    uiKeyIsAudioPauseKey (uiwcont_t *uiwidget);
bool    uiKeyIsAudioNextKey (uiwcont_t *uiwidget);
bool    uiKeyIsAudioPrevKey (uiwcont_t *uiwidget);
bool    uiKeyIsUpKey (uiwcont_t *uiwidget);
bool    uiKeyIsDownKey (uiwcont_t *uiwidget);
bool    uiKeyIsPageUpDownKey (uiwcont_t *uiwidget);
bool    uiKeyIsNavKey (uiwcont_t *uiwidget);
bool    uiKeyIsMaskedKey (uiwcont_t *uiwidget);
bool    uiKeyIsControlPressed (uiwcont_t *uiwidget);
bool    uiKeyIsShiftPressed (uiwcont_t *uiwidget);

#endif /* INC_UIKEYS_H */
