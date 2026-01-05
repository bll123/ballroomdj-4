/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mdebug.h"
#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uievents.h"

typedef struct uievent {
  int         junk;
} uievent_t;

uiwcont_t *
uiEventAlloc (void)
{
  return NULL;
}

void
uiEventFree (uiwcont_t *uiwidget)
{
  return;
}

uiwcont_t *
uiEventCreateEventBox (uiwcont_t *uiwidgetp)
{
  return NULL;
}

void
uiEventSetKeyCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb)
{
  return;
}

void
uiEventSetButtonCallback (uiwcont_t *uieventwidget,
    uiwcont_t *uiwidgetp, callback_t *uicb)
{
  return;
}

void
uiEventSetScrollCallback (uiwcont_t *uieventwidget,
    uiwcont_t *uiwidgetp, callback_t *uicb)
{
  return;
}

void
uiEventSetMotionCallback (uiwcont_t *uieventwidget,
    uiwcont_t *uiwidgetp, callback_t *uicb)
{
  return;
}

bool
uiEventIsKeyPressEvent (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsKeyReleaseEvent (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsButtonPressEvent (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsButtonDoublePressEvent (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsButtonReleaseEvent (uiwcont_t *uiwidget)   /* KEEP */
{
  return false;
}

bool
uiEventIsMovementKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsKey (uiwcont_t *uiwidget, unsigned char keyval)
{
  return false;
}

bool
uiEventIsEnterKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsAudioPlayKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsAudioPauseKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsAudioNextKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsAudioPrevKey (uiwcont_t *uiwidget)
{
  return false;
}

/* includes page up */
bool
uiEventIsUpKey (uiwcont_t *uiwidget)
{
  return false;
}

/* includes page down */
bool
uiEventIsDownKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsPageUpDownKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsNavKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsPasteCutKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsMaskedKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsAltPressed (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsControlPressed (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiEventIsShiftPressed (uiwcont_t *uiwidget)
{
  return false;
}

int
uiEventGetButton (uiwcont_t *uiwidget)
{
  return -1;
}

