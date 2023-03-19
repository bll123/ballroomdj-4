/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include "ui.h"

void uiutilsAddAccentColorDisplay (UIWidget *vbox, uiwidget_t *hbox, uiwidget_t *uiwidget);
void uiutilsSetAccentColor (UIWidget *uiwidgetp);
const char * uiutilsGetCurrentFont (void);

#endif /* INC_UIUTILS_H */
