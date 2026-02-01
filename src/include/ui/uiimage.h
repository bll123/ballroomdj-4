/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiImageNew (void);
void  uiImageFree (uiwcont_t *uiwidget);
uiwcont_t *uiImageFromFile (const char *fn);
uiwcont_t *uiImageScaledFromFile (const char *fn, int scale);
void  uiImageClear (uiwcont_t *uiwidget);
void  uiImageCopy (uiwcont_t *toimg, uiwcont_t *fromimg);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

