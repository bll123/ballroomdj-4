/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGENERAL_H
#define INC_UIGENERAL_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

/* general routines that are called by the ui specific code */
void uiutilsUIWidgetInit (UIWidget *uiwidget);
bool uiutilsUIWidgetSet (UIWidget *uiwidget);
void uiutilsUIWidgetCopy (UIWidget *target, UIWidget *source);

#endif /* INC_UIGENERAL_H */
