/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UITOGGLE_H
#define INC_UITOGGLE_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "callback.h"

void uiCreateCheckButton (uiwidget_t *uiwidget, const char *txt, int value);
void uiCreateRadioButton (uiwidget_t *uiwidget, uiwidget_t *widgetgrp, const char *txt, int value);
void uiCreateToggleButton (uiwidget_t *uiwidget, const char *txt, const char *imgname,
    const char *tooltiptxt, uiwidget_t *image, int value);
void uiToggleButtonSetCallback (uiwidget_t *uiwidget, callback_t *uicb);
void uiToggleButtonSetImage (uiwidget_t *uiwidget, uiwidget_t *image);
bool uiToggleButtonIsActive (uiwidget_t *uiwidget);
void uiToggleButtonSetState (uiwidget_t *uiwidget, int state);

#endif /* INC_UITOGGLE_H */
