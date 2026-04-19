
/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t * uiCreateGrid (void);
void uiGridPack (uiwcont_t *uigrid, uiwcont_t *uiwidget, int row, int col);
void uiGridPackSize (uiwcont_t *uigrid, uiwcont_t *uiwidget, int row, int col, int numrows, int numcols);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif
