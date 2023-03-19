/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIIMAGE_H
#define INC_UIIMAGE_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void  uiImageFromFile (uiwcont_t *uiwidget, const char *fn);
void  uiImagePersistentFromFile (uiwcont_t *uiwidget, const char *fn);
void  uiImagePersistentFree (uiwcont_t *uiwidget);
void  uiImageNew (uiwcont_t *uiwidget);
void  uiImageFromFile (uiwcont_t *uiwidget, const char *fn);
void  uiImageScaledFromFile (uiwcont_t *uiwidget, const char *fn, int scale);
void  uiImageClear (uiwcont_t *uiwidget);
void  uiImageConvertToPixbuf (uiwcont_t *uiwidget);
void  uiImageSetFromPixbuf (uiwcont_t *uiwidget, uiwcont_t *uipixbuf);
void  *uiImageGetPixbuf (uiwcont_t *uiwidget);

#endif /* INC_UIIMAGE_H */
