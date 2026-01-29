/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateVertBox (void);
uiwcont_t *uiCreateHorizBox (void);
void uiBoxFree (uiwcont_t *uibox);
void uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackStartExpand (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackEndExpand (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxSetSizeChgCallback (uiwcont_t *uiwidget, callback_t *uicb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

