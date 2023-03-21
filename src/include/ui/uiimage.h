/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIIMAGE_H
#define INC_UIIMAGE_H

#include "uiwcont.h"

uiwcont_t *uiImageNew (void);
void  uiImageFromFile (uiwcont_t *uiwidget, const char *fn);
void  uiImageScaledFromFile (uiwcont_t *uiwidget, const char *fn, int scale);
void  uiImagePersistentFromFile (uiwcont_t *uiwidget, const char *fn);
void  uiImagePersistentFree (uiwcont_t *uiwidget);
void  uiImageClear (uiwcont_t *uiwidget);
void  uiImageConvertToPixbuf (uiwcont_t *uiwidget);
void  uiImageSetFromPixbuf (uiwcont_t *uiwidget, uiwcont_t *uipixbuf);
void  *uiImageGetPixbuf (uiwcont_t *uiwidget);

#endif /* INC_UIIMAGE_H */
