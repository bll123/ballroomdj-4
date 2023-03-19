/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIIMAGE_H
#define INC_UIIMAGE_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void  uiImageFromFile (uiwidget_t *uiwidget, const char *fn);
void  uiImagePersistentFromFile (uiwidget_t *uiwidget, const char *fn);
void  uiImagePersistentFree (uiwidget_t *uiwidget);
void  uiImageNew (uiwidget_t *uiwidget);
void  uiImageFromFile (uiwidget_t *uiwidget, const char *fn);
void  uiImageScaledFromFile (uiwidget_t *uiwidget, const char *fn, int scale);
void  uiImageClear (uiwidget_t *uiwidget);
void  uiImageConvertToPixbuf (uiwidget_t *uiwidget);
void  uiImageSetFromPixbuf (uiwidget_t *uiwidget, uiwidget_t *uipixbuf);
void  *uiImageGetPixbuf (uiwidget_t *uiwidget);

#endif /* INC_UIIMAGE_H */
