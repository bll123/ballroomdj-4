/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  UIEVENT_EV_NONE,
  UIEVENT_EV_KEY_PRESS,
  UIEVENT_EV_KEY_RELEASE,
  UIEVENT_EV_MBUTTON_PRESS,
  UIEVENT_EV_MBUTTON_DOUBLE_PRESS,
  UIEVENT_EV_MBUTTON_RELEASE,
  UIEVENT_EV_SCROLL,
};

enum {
  /* these must match what gtk returns */
  UIEVENT_BUTTON_1 = 1,
  UIEVENT_BUTTON_2 = 2,
  UIEVENT_BUTTON_3 = 3,
  UIEVENT_BUTTON_4 = 4,
  UIEVENT_BUTTON_5 = 5,
  UIEVENT_DIR_PREV = 0,
  UIEVENT_DIR_NEXT = 1,
  UIEVENT_DIR_LEFT = 2,
  UIEVENT_DIR_RIGHT = 3,
};

uiwcont_t * uiEventAlloc (void);
void    uiEventFree (uiwcont_t *uiwidget);
uiwcont_t * uiEventCreateEventBox (uiwcont_t *uiwidgetp);
void    uiEventSetKeyCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb);
void    uiEventSetButtonCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb);
void    uiEventSetScrollCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb);
void    uiEventSetMotionCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb);
int     uiEventEvent (uiwcont_t *uiwidget);
bool    uiEventIsKeyPressEvent (uiwcont_t *uiwidget);
bool    uiEventIsKeyReleaseEvent (uiwcont_t *uiwidget);
bool    uiEventIsButtonPressEvent (uiwcont_t *uiwidget);
bool    uiEventIsButtonDoublePressEvent (uiwcont_t *uiwidget);
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
int     uiEventGetButton (uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

