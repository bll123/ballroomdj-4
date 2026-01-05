/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateLink (const char *label, const char *uri);
void uiLinkSet (uiwcont_t *uilink, const char *label, const char *uri);
void uiLinkSetActivateCallback (uiwcont_t *uilink, callback_t *uicb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

