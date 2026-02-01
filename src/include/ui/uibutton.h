/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  BUTTON_ON = true,
  BUTTON_OFF = false,
};

uiwcont_t *uiCreateButton (callback_t *uicb, const char *title, const char *imagenm, const char *tooltiptxt);
void uiButtonFree (uiwcont_t *uiwidget);
void uiButtonSetImagePosRight (uiwcont_t *uiwidget);
void uiButtonSetImageMarginTop (uiwcont_t *uiwidget, int margin);
void uiButtonSetImageIcon (uiwcont_t *uiwidget, const char *nm);
void uiButtonSetAltImage (uiwcont_t *uiwidget, const char *imagenm);
void uiButtonSetState (uiwcont_t *uiwidget, int newstate);
void uiButtonAlignLeft (uiwcont_t *uiwidget);
void uiButtonSetText (uiwcont_t *uiwidget, const char *txt);
void uiButtonSetReliefNone (uiwcont_t *uiwidget);
void uiButtonSetFlat (uiwcont_t *uiwidget);
void uiButtonSetRepeat (uiwcont_t *uiwidget, int repeatms);
int uiButtonGetState (uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

