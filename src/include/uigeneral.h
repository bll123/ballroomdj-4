/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGENERAL_H
#define INC_UIGENERAL_H

#define MACOS_UI_DEBUG 1

#if BDJ4_UI_GTK3
# include "ui-gtk3.h"
#endif
#if BDJ4_UI_NULL
# include "ui-null.h"
#endif
#if BDJ4_UI_MACOS
# include "ui-macos.h"
#endif

#include "uiwcont.h"

/* uibutton.c */
bool uiButtonCheckRepeat (uiwcont_t *uiwidget);
bool uiButtonPressCallback (void *udata);
bool uiButtonReleaseCallback (void *udata);

/* uilabel.c */
uiwcont_t * uiCreateColonLabel (const char *txt);

/* uifont.c */
void uiFontInfo (const char *font, char *buff, size_t sz, int *fontsz);

#endif /* INC_UIGENERAL_H */
