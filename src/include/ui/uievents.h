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

enum {
  UIEVENT_BUTTON_1 = 1,
  UIEVENT_BUTTON_2 = 2,
  UIEVENT_BUTTON_3 = 3,
  UIEVENT_BUTTON_4 = 4,
  UIEVENT_BUTTON_5 = 5,
  UIEVENT_ENTER = true,
  UIEVENT_LEAVE = false,
};

uiwcont_t * uiEventAlloc (void);
void    uiEventFree (uiwcont_t *uiwidget);
uiwcont_t * uiEventCreateEventBox (uiwcont_t *uiwidgetp);
void    uiEventSetKeyCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb);
void    uiEventSetButtonCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb);
void    uiEventSetScrollCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb);
void    uiEventSetEnterLeaveCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb);
int     uiEventEvent (uiwcont_t *uiwidget);
bool    uiEventIsKeyPressEvent (uiwcont_t *uiwidget);
bool    uiEventIsKeyReleaseEvent (uiwcont_t *uiwidget);
bool    uiEventIsButtonPressEvent (uiwcont_t *uiwidget);
bool    uiEventIsButtonReleaseEvent (uiwcont_t *uiwidget);
bool    uiEventIsMovementKey (uiwcont_t *uiwidget);
bool    uiEventIsKey (uiwcont_t *uiwidget, unsigned char keyval);
bool    uiEventIsEnterKey (uiwcont_t *uiwidget);
bool    uiEventIsAudioPlayKey (uiwcont_t *uiwidget);
bool    uiEventIsAudioPauseKey (uiwcont_t *uiwidget);
bool    uiEventIsAudioNextKey (uiwcont_t *uiwidget);
bool    uiEventIsAudioPrevKey (uiwcont_t *uiwidget);
bool    uiEventIsUpKey (uiwcont_t *uiwidget);
bool    uiEventIsDownKey (uiwcont_t *uiwidget);
bool    uiEventIsPageUpDownKey (uiwcont_t *uiwidget);
bool    uiEventIsNavKey (uiwcont_t *uiwidget);
bool    uiEventIsPasteCutKey (uiwcont_t *uiwidget);
bool    uiEventIsMaskedKey (uiwcont_t *uiwidget);
bool    uiEventIsAltPressed (uiwcont_t *uiwidget);
bool    uiEventIsControlPressed (uiwcont_t *uiwidget);
bool    uiEventIsShiftPressed (uiwcont_t *uiwidget);
int     uiEventButtonPressed (uiwcont_t *uiwidget);
bool    uiEventCheckWidget (uiwcont_t *keywcont, uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIKEYS_H */
