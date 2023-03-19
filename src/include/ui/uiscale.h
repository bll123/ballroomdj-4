/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISCALE_H
#define INC_UISCALE_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "callback.h"

void    uiCreateScale (uiwidget_t *uiwidget, double lower, double upper,
    double stepinc, double pageinc, double initvalue, int digits);
void    uiScaleSetCallback (uiwidget_t *uiscale, callback_t *uicb);
double  uiScaleEnforceMax (uiwidget_t *uiscale, double value);
double  uiScaleGetValue (uiwidget_t *uiscale);
int     uiScaleGetDigits (uiwidget_t *uiscale);
void    uiScaleSetValue (uiwidget_t *uiscale, double value);
void    uiScaleSetRange (uiwidget_t *uiscale, double start, double end);
void    uiScaleSetState (uiwidget_t *uiscale, int state);

#endif /* INC_UISCALE_H */
