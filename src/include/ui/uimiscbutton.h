/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateFontButton (const char *fontname);
const char * uiFontButtonGetFont (uiwcont_t *uiwidget);
uiwcont_t *uiCreateColorButton (const char *color);
void uiColorButtonGetColor (uiwcont_t *uiwidget, char *tbuff, size_t sz);
void uiColorButtonSetColor (uiwcont_t *uiwidget, const char *color);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

