/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include "ui.h"

void uiutilsAddAccentColorDisplay (uiwcont_t *vbox, uiwcont_t *hbox, uiwcont_t *uiwidget);
void uiutilsSetAccentColor (uiwcont_t *uiwidgetp);
const char * uiutilsGetCurrentFont (void);

#endif /* INC_UIUTILS_H */
