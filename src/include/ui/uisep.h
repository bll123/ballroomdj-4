/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISEP_H
#define INC_UISEP_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void uiCreateHorizSeparator (uiwidget_t *uiwidget);
void uiSeparatorAddClass (const char *classnm, const char *color);

#endif /* INC_UISEP_H */
