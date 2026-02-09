/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */

#pragma once

#include <stdint.h>

#include "nodiscard.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct callback callback_t;

typedef bool  (*callbackFunc)(void *udata);
typedef bool  (*callbackFuncD)(void *udata, double value);
typedef bool  (*callbackFuncI)(void *udata, int32_t value);
typedef bool  (*callbackFuncII)(void *udata, int32_t a, int32_t b);
typedef int32_t (*callbackFuncSS)(void *udata, const char *a, const char *b);
typedef int32_t (*callbackFuncS)(void *udata, const char *txt);
typedef bool  (*callbackFuncSI)(void *udata, const char *txt, int32_t value);

/* uicallback.c */
bool callbackIsSet (callback_t *cb);

bool callbackHandler (callback_t *cb);
bool callbackHandlerD (callback_t *cb, double value);
bool callbackHandlerI (callback_t *cb, int32_t value);
bool callbackHandlerII (callback_t *cb, int32_t a, int32_t b);
int32_t callbackHandlerS (callback_t *cb, const char *str);
int32_t callbackHandlerSS (callback_t *cb, const char *a, const char *b);
bool callbackHandlerSI (callback_t *cb, const char *str, int32_t value);

void callbackFree (callback_t *cb);
BDJ_NODISCARD callback_t *callbackInit (callbackFunc cb, void *udata, const char *actiontext);
BDJ_NODISCARD callback_t *callbackInitD (callbackFuncD cb, void *udata);
BDJ_NODISCARD callback_t *callbackInitI (callbackFuncI cb, void *udata);
BDJ_NODISCARD callback_t *callbackInitII (callbackFuncII cb, void *udata);
BDJ_NODISCARD callback_t *callbackInitS (callbackFuncS cb, void *udata);
BDJ_NODISCARD callback_t *callbackInitSS (callbackFuncSS cb, void *udata);
BDJ_NODISCARD callback_t *callbackInitSI (callbackFuncSI cb, void *udata);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

