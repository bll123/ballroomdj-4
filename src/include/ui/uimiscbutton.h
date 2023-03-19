/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIMISCBUTTON_H
#define INC_UIMISCBUTTON_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void uiCreateFontButton (uiwcont_t *uiwidget, const char *fontname);
const char * uiFontButtonGetFont (uiwcont_t *uiwidget);
void uiCreateColorButton (uiwcont_t *uiwidget, const char *color);
void uiColorButtonGetColor (uiwcont_t *uiwidget, char *tbuff, size_t sz);

#endif /* INC_UIMISCBUTTON_H */
