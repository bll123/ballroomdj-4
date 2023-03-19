/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include "ui.h"

void uiutilsAddAccentColorDisplay (uiwidget_t *vbox, uiwidget_t *hbox, uiwidget_t *uiwidget);
void uiutilsSetAccentColor (uiwidget_t *uiwidgetp);
const char * uiutilsGetCurrentFont (void);

#endif /* INC_UIUTILS_H */
