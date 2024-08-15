/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIIMAGE_H
#define INC_UIIMAGE_H

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiImageNew (void);
uiwcont_t *uiImageFromFile (const char *fn);
uiwcont_t *uiImageScaledFromFile (const char *fn, int scale);
void  uiImageClear (uiwcont_t *uiwidget);
void  uiImageConvertToPixbuf (uiwcont_t *uiwidget);
void  uiImageSetFromPixbuf (uiwcont_t *uiwidget, uiwcont_t *uipixbuf);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIIMAGE_H */
