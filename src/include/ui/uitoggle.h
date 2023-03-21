/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UITOGGLE_H
#define INC_UITOGGLE_H

#include "callback.h"
#include "uiwcont.h"

void uiCreateCheckButton (uiwcont_t *uiwidget, const char *txt, int value);
uiwcont_t *uiCreateRadioButton (uiwcont_t *widgetgrp, const char *txt, int value);
uiwcont_t *uiCreateToggleButton (const char *txt, const char *imgname,
    const char *tooltiptxt, uiwcont_t *image, int value);
void uiToggleButtonSetCallback (uiwcont_t *uiwidget, callback_t *uicb);
void uiToggleButtonSetImage (uiwcont_t *uiwidget, uiwcont_t *image);
bool uiToggleButtonIsActive (uiwcont_t *uiwidget);
void uiToggleButtonSetState (uiwcont_t *uiwidget, int state);

#endif /* INC_UITOGGLE_H */
