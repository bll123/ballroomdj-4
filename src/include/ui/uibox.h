/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIBOX_H
#define INC_UIBOX_H

#include "uiwcont.h"

void uiCreateVertBox (uiwcont_t *uiwidget);
void uiCreateHorizBox (uiwcont_t *uiwidget);
void uiBoxPackInWindow (uiwcont_t *uiwindow, uiwcont_t *uibox);
void uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackStartExpand (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget);
void uiBoxPackEndExpand (uiwcont_t *uibox, uiwcont_t *uiwidget);

#endif /* INC_UIBOX_H */
