/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uitoggle.h"

uiwcont_t *
uiCreateCheckButton (const char *txt, int value)
{
  return NULL;
}

uiwcont_t *
uiCreateRadioButton (uiwcont_t *widgetgrp, const char *txt, int value)
{
  return NULL;
}

uiwcont_t *
uiCreateToggleButton (const char *txt,
    const char *imgname, const char *tooltiptxt, uiwcont_t *image, int value)
{
  return NULL;
}

void
uiToggleButtonSetCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

void
uiToggleButtonSetImage (uiwcont_t *uiwidget, uiwcont_t *image)
{
  return;
}

void
uiToggleButtonSetText (uiwcont_t *uiwidget, const char *txt)
{
  return;
}

bool
uiToggleButtonIsActive (uiwcont_t *uiwidget)
{
  return false;
}

void
uiToggleButtonSetState (uiwcont_t *uiwidget, int state)
{
  return;
}
