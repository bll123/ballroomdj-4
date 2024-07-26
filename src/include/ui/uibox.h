/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIBOX_H
#define INC_UIBOX_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "callback.h"
#include "uiwcont.h"

uiwcont_t *uiCreateVertBox (void);
uiwcont_t *uiCreateHorizBox (void);
void uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackStartExpand (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackEndExpand (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxSetSizeChgCallback (uiwcont_t *uiwidget, callback_t *uicb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIBOX_H */
