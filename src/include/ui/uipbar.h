/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIPBAR_H
#define INC_UIPBAR_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void uiCreateProgressBar (uiwcont_t *uiwidget);
void uiProgressBarSet (uiwcont_t *uipb, double val);

#endif /* INC_UIPBAR_H */
