/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISIZEGRP_H
#define INC_UISIZEGRP_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void uiCreateSizeGroupHoriz (UIWidget *);
void uiSizeGroupAdd (UIWidget *uiw, UIWidget *uiwidget);

#endif /* INC_UISIZEGRP_H */
