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

uiwcont_t *
uiKeyAlloc (void)
{
  return NULL;
}

void
uiKeyFree (uiwcont_t *uiwidget)
{
  return;
}

void
uiKeySetKeyCallback (uiwcont_t *uiwidget, uiwcont_t *uiwidgetp, callback_t *uicb)
{
  return;
}

bool
uiKeyIsPressEvent (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiKeyIsReleaseEvent (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiKeyIsMovementKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiKeyIsKey (uiwcont_t *uiwidget, unsigned char keyval)
{
  return false;
}

bool
uiKeyIsAudioPlayKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiKeyIsAudioPauseKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiKeyIsAudioNextKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiKeyIsAudioPrevKey (uiwcont_t *uiwidget)
{
  return false;
}

/* includes page up */
bool
uiKeyIsUpKey (uiwcont_t *uiwidget)
{
  return false;
}

/* includes page down */
bool
uiKeyIsDownKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiKeyIsPageUpDownKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiKeyIsNavKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiKeyIsMaskedKey (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiKeyIsControlPressed (uiwcont_t *uiwidget)
{
  return false;
}

bool
uiKeyIsShiftPressed (uiwcont_t *uiwidget)
{
  return false;
}

