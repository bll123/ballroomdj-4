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
uiwcont_t *uiCreateVertBox_r (void);
uiwcont_t *uiCreateHorizBox_r (void);
void uiBoxSetSizeChgCallback (uiwcont_t *uiwidget, callback_t *uicb);

void uiBoxPostProcess_r (uiwcont_t *uibox, uiwcont_t *prev, uiwcont_t *curr, uiwcont_t *next);

/* real box pack */
void uiBoxPackStart_r (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackStartExpandChildren_r (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackEnd_r (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackEndExpandChildren_r (uiwcont_t *uibox, uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

