/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIBUTTON_H
#define INC_UIBUTTON_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "callback.h"
#include "uiwcont.h"

uiwcont_t *uiCreateButton (callback_t *uicb, char *title, char *imagenm);
void uiButtonFree (uiwcont_t *uiwidget);
void uiButtonSetImagePosRight (uiwcont_t *uiwidget);
void uiButtonSetImage (uiwcont_t *uiwidget, const char *imagenm, const char *tooltip);
void uiButtonSetImageIcon (uiwcont_t *uiwidget, const char *nm);
void uiButtonAlignLeft (uiwcont_t *uiwidget);
void uiButtonSetText (uiwcont_t *uiwidget, const char *txt);
void uiButtonSetReliefNone (uiwcont_t *uiwidget);
void uiButtonSetFlat (uiwcont_t *uiwidget);
void uiButtonSetRepeat (uiwcont_t *uiwidget, int repeatms);
bool uiButtonCheckRepeat (uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIBUTTON_H */
