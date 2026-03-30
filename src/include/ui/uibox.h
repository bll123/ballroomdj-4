/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* real create vert/horiz box */
uiwcont_t *ruiCreateVertBox (void);
uiwcont_t *ruiCreateHorizBox (void);
void uiBoxPostProcess (uiwcont_t *uibox);
void uiBoxSetSizeChgCallback (uiwcont_t *uiwidget, callback_t *uicb);
void uiBoxPostProcess (uiwcont_t *uiwidget);

/* real box pack */
void ruiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget);
void ruiBoxPackStartExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget);
void ruiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget);
void ruiBoxPackEndExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

