/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIPANEDWINDOW_H
#define INC_UIPANEDWINDOW_H

#include "uiwcont.h"

uiwcont_t *uiPanedWindowCreateHoriz (void);
uiwcont_t *uiPanedWindowCreateVert (void);
void uiPanedWindowPackStart (uiwcont_t *panedwin, uiwcont_t *box);
void uiPanedWindowPackEnd (uiwcont_t *panedwin, uiwcont_t *box);
int uiPanedWindowGetPosition (uiwcont_t *panedwin);
void uiPanedWindowSetPosition (uiwcont_t *panedwin, int position);

#endif /* INC_UIPANEDWINDOW_H */
