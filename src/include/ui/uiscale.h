/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateScale (const char *ident, double lower, double upper,
    double stepinc, double pageinc, double initvalue, int digits);
void    uiScaleSetCallback (uiwcont_t *uiscale, callback_t *uicb);
double  uiScaleEnforceMax (uiwcont_t *uiscale, double value);
double  uiScaleGetValue (uiwcont_t *uiscale);
int     uiScaleGetDigits (uiwcont_t *uiscale);
void    uiScaleSetValue (uiwcont_t *uiscale, double value);
void    uiScaleSetRange (uiwcont_t *uiscale, double start, double end);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

