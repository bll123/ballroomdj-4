/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISCALE_H
#define INC_UISCALE_H

#include "callback.h"
#include "uiwcont.h"

uiwcont_t *uiCreateScale (double lower, double upper,
    double stepinc, double pageinc, double initvalue, int digits);
void    uiScaleSetCallback (uiwcont_t *uiscale, callback_t *uicb);
double  uiScaleEnforceMax (uiwcont_t *uiscale, double value);
double  uiScaleGetValue (uiwcont_t *uiscale);
int     uiScaleGetDigits (uiwcont_t *uiscale);
void    uiScaleSetValue (uiwcont_t *uiscale, double value);
void    uiScaleSetRange (uiwcont_t *uiscale, double start, double end);
void    uiScaleSetState (uiwcont_t *uiscale, int state);

#endif /* INC_UISCALE_H */
