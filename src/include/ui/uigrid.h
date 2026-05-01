/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t * uiCreateGrid_r (void);
void uiGridFree_r (uiwcont_t *uigrid);
void uiGridPackLoc_r (uiwcont_t *uigrid, uiwcont_t *uiwidget, int row, int col, int numcols);
void uiGridPostProcess_r (uiwcont_t *uigrid, int row, int col, uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif
