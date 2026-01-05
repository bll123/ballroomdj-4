/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateCheckButton (const char *txt, int value);
uiwcont_t *uiCreateRadioButton (uiwcont_t *widgetgrp, const char *txt, int value);
uiwcont_t *uiCreateToggleButton (const char *txt, const char *imgname,
    const char *tooltiptxt, uiwcont_t *image, int value);
void uiToggleButtonSetCallback (uiwcont_t *uiwidget, callback_t *uicb);
void uiToggleButtonSetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb);
void uiToggleButtonSetImage (uiwcont_t *uiwidget, uiwcont_t *image);
void uiToggleButtonSetText (uiwcont_t *uiwidget, const char *txt);
bool uiToggleButtonIsActive (uiwcont_t *uiwidget);
void uiToggleButtonSetValue (uiwcont_t *uiwidget, int state);
void uiToggleButtonEllipsize (uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

