/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIKEYS_H
#define INC_UIKEYS_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "callback.h"

typedef struct uikey uikey_t;

uikey_t * uiKeyAlloc (void);
void    uiKeyFree (uikey_t *uikey);
void    uiKeySetKeyCallback (uikey_t *uikey, uiwidget_t *uiwidgetp, callback_t *uicb);
int     uiKeyEvent (uikey_t *uikey);
bool    uiKeyIsPressEvent (uikey_t *uikey);
bool    uiKeyIsReleaseEvent (uikey_t *uikey);
bool    uiKeyIsMovementKey (uikey_t *uikey);
bool    uiKeyIsKey (uikey_t *uikey, unsigned char keyval);
bool    uiKeyIsAudioPlayKey (uikey_t *uikey);
bool    uiKeyIsAudioPauseKey (uikey_t *uikey);
bool    uiKeyIsAudioNextKey (uikey_t *uikey);
bool    uiKeyIsAudioPrevKey (uikey_t *uikey);
bool    uiKeyIsUpKey (uikey_t *uikey);
bool    uiKeyIsDownKey (uikey_t *uikey);
bool    uiKeyIsPageUpDownKey (uikey_t *uikey);
bool    uiKeyIsNavKey (uikey_t *uikey);
bool    uiKeyIsMaskedKey (uikey_t *uikey);
bool    uiKeyIsControlPressed (uikey_t *uikey);
bool    uiKeyIsShiftPressed (uikey_t *uikey);

#endif /* INC_UIKEYS_H */
