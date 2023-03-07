/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIBOX_H
#define INC_UIBOX_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void uiCreateVertBox (UIWidget *uiwidget);
void uiCreateHorizBox (UIWidget *uiwidget);
void uiBoxPackInWindow (UIWidget *uiwindow, UIWidget *uibox);
void uiBoxPackStart (UIWidget *uibox, UIWidget *uiwidget);
void uiBoxPackStartExpand (UIWidget *uibox, UIWidget *uiwidget);
void uiBoxPackEnd (UIWidget *uibox, UIWidget *uiwidget);
void uiBoxPackEndExpand (UIWidget *uibox, UIWidget *uiwidget);

#endif /* INC_UIBOX_H */
