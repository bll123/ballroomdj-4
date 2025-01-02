/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIMISCBUTTON_H
#define INC_UIMISCBUTTON_H

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateFontButton (const char *fontname);
const char * uiFontButtonGetFont (uiwcont_t *uiwidget);
uiwcont_t *uiCreateColorButton (const char *color);
void uiColorButtonGetColor (uiwcont_t *uiwidget, char *tbuff, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIMISCBUTTON_H */
