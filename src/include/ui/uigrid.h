/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t * uiCreateGrid (void);
void uiGridFree (uiwcont_t *uigrid);
void uiGridPack (uiwcont_t *uigrid, uiwcont_t *uiwidget);
void uiGridPackNoExpand (uiwcont_t *uigrid, uiwcont_t *uiwidget);
void uiGridPackSize (uiwcont_t *uigrid, uiwcont_t *uiwidget, int numrows, int numcols);
void uiGridNewRow (uiwcont_t *uigrid);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif
