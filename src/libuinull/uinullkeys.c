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

uikey_t *
uiKeyAlloc (void)
{
  return NULL;
}

void
uiKeyFree (uikey_t *uikey)
{
  return;
}

void
uiKeySetKeyCallback (uikey_t *uikey, uiwcont_t *uiwidgetp, callback_t *uicb)
{
  return;
}

bool
uiKeyIsPressEvent (uikey_t *uikey)
{
  return false;
}

bool
uiKeyIsReleaseEvent (uikey_t *uikey)
{
  return false;
}

bool
uiKeyIsMovementKey (uikey_t *uikey)
{
  return false;
}

bool
uiKeyIsKey (uikey_t *uikey, unsigned char keyval)
{
  return false;
}

bool
uiKeyIsAudioPlayKey (uikey_t *uikey)
{
  return false;
}

bool
uiKeyIsAudioPauseKey (uikey_t *uikey)
{
  return false;
}

bool
uiKeyIsAudioNextKey (uikey_t *uikey)
{
  return false;
}

bool
uiKeyIsAudioPrevKey (uikey_t *uikey)
{
  return false;
}

/* includes page up */
bool
uiKeyIsUpKey (uikey_t *uikey)
{
  return false;
}

/* includes page down */
bool
uiKeyIsDownKey (uikey_t *uikey)
{
  return false;
}

bool
uiKeyIsPageUpDownKey (uikey_t *uikey)
{
  return false;
}

bool
uiKeyIsNavKey (uikey_t *uikey)
{
  return false;
}

bool
uiKeyIsMaskedKey (uikey_t *uikey)
{
  return false;
}

bool
uiKeyIsControlPressed (uikey_t *uikey)
{
  return false;
}

bool
uiKeyIsShiftPressed (uikey_t *uikey)
{
  return false;
}

