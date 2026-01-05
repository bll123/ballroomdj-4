/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiPanedWindowCreateVert (void);
void uiPanedWindowPackStart (uiwcont_t *panedwin, uiwcont_t *box);
void uiPanedWindowPackEnd (uiwcont_t *panedwin, uiwcont_t *box);
int uiPanedWindowGetPosition (uiwcont_t *panedwin);
void uiPanedWindowSetPosition (uiwcont_t *panedwin, int position);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

