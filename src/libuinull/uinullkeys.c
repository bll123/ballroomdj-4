/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

#include "ui/uikeys.h"

typedef struct uikey {
  int         junk;
} uikey_t;

uiwidget_t *
uiKeyAlloc (void)
{
  return NULL;
}

void
uiKeyFree (uiwidget_t *uiwidget)
{
  return;
}

void
uiKeySetKeyCallback (uiwidget_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb)
{
  return;
}

bool
uiKeyIsPressEvent (uiwidget_t *uiwidget)
{
  return false;
}

bool
uiKeyIsReleaseEvent (uiwidget_t *uiwidget)
{
  return false;
}

bool
uiKeyIsMovementKey (uiwidget_t *uiwidget)
{
  return false;
}

bool
uiKeyIsKey (uiwidget_t *uiwidget, unsigned char keyval)
{
  return false;
}

bool
uiKeyIsAudioPlayKey (uiwidget_t *uiwidget)
{
  return false;
}

bool
uiKeyIsAudioPauseKey (uiwidget_t *uiwidget)
{
  return false;
}

bool
uiKeyIsAudioNextKey (uiwidget_t *uiwidget)
{
  return false;
}

bool
uiKeyIsAudioPrevKey (uiwidget_t *uiwidget)
{
  return false;
}

/* includes page up */
bool
uiKeyIsUpKey (uiwidget_t *uiwidget)
{
  return false;
}

/* includes page down */
bool
uiKeyIsDownKey (uiwidget_t *uiwidget)
{
  return false;
}

bool
uiKeyIsPageUpDownKey (uiwidget_t *uiwidget)
{
  return false;
}

bool
uiKeyIsNavKey (uiwidget_t *uiwidget)
{
  return false;
}

bool
uiKeyIsMaskedKey (uiwidget_t *uiwidget)
{
  return false;
}

bool
uiKeyIsControlPressed (uiwidget_t *uiwidget)
{
  return false;
}

bool
uiKeyIsShiftPressed (uiwidget_t *uiwidget)
{
  return false;
}

