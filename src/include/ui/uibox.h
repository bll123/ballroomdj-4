/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIBOX_H
#define INC_UIBOX_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void uiCreateVertBox (uiwidget_t *uiwidget);
void uiCreateHorizBox (uiwidget_t *uiwidget);
void uiBoxPackInWindow (uiwidget_t *uiwindow, uiwidget_t *uibox);
void uiBoxPackStart (uiwidget_t *uibox, uiwidget_t *uiwidget);
void uiBoxPackStartExpand (uiwidget_t *uibox, uiwidget_t *uiwidget);
void uiBoxPackEnd (uiwidget_t *uibox, uiwidget_t *uiwidget);
void uiBoxPackEndExpand (uiwidget_t *uibox, uiwidget_t *uiwidget);

#endif /* INC_UIBOX_H */
