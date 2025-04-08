/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIBUTTON_H
#define INC_UIBUTTON_H

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateButton (const char *ident, callback_t *uicb, const char *title, char *imagenm);
void uiButtonFree (uiwcont_t *uiwidget);
void uiButtonSetImagePosRight (uiwcont_t *uiwidget);
void uiButtonSetImageMarginTop (uiwcont_t *uiwidget, int margin);
void uiButtonSetImageIcon (uiwcont_t *uiwidget, const char *nm);
void uiButtonAlignLeft (uiwcont_t *uiwidget);
void uiButtonSetText (uiwcont_t *uiwidget, const char *txt);
void uiButtonSetReliefNone (uiwcont_t *uiwidget);
void uiButtonSetFlat (uiwcont_t *uiwidget);
void uiButtonSetRepeat (uiwcont_t *uiwidget, int repeatms);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIBUTTON_H */
