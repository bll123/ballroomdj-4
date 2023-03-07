/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIIMAGE_H
#define INC_UIIMAGE_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void  uiImageFromFile (UIWidget *uiwidget, const char *fn);
void  uiImagePersistentFromFile (UIWidget *uiwidget, const char *fn);
void  uiImagePersistentFree (UIWidget *uiwidget);
void  uiImageNew (UIWidget *uiwidget);
void  uiImageFromFile (UIWidget *uiwidget, const char *fn);
void  uiImageScaledFromFile (UIWidget *uiwidget, const char *fn, int scale);
void  uiImageClear (UIWidget *uiwidget);
void  uiImageGetPixbuf (UIWidget *uiwidget);
void  uiImageSetFromPixbuf (UIWidget *uiwidget, UIWidget *uipixbuf);

#endif /* INC_UIIMAGE_H */
