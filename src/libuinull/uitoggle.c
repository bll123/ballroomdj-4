/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
    const char *imgname, const char *tooltiptxt, int value)
{
  return NULL;
}

void
uiToggleButtonFree (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "toggle-free")) {
    return;
  }
}

void
uiToggleButtonSetAltImage (uiwcont_t *uiwidget, const char *imagenm)
{
  uitoggle_t      *uitoggle;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "toggle-set-alt-image")) {
    return;
  }

  uitoggle = uiwidget->uiint.uitoggle;

  if (imagenm != NULL) {
    ;
  }
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
uiToggleButtonSetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "tb-set-cb")) {
    return;
  }
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
uiToggleButtonSetValue (uiwcont_t *uiwidget, int state)
{
  return;
}

void
uiToggleButtonEllipsize (uiwcont_t *uiwidget)
{
  return;
}

