/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIADJUSTMENT_H
#define INC_UIADJUSTMENT_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void  uiCreateAdjustment (UIWidget *uiwidget, double value, double start, double end, double stepinc, double pageinc, double pagesz);
void * uiAdjustmentGetAdjustment (UIWidget *uiwidget);

#endif /* INC_UIADJUSTMENT_H */
