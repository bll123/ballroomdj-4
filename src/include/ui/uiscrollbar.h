/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISCROLLBAR_H
#define INC_UISCROLLBAR_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void uiCreateVerticalScrollbar (uiwidget_t *uiwidget, double upper);
void uiScrollbarSetUpper (uiwidget_t *uisb, double upper);
void uiScrollbarSetPosition (uiwidget_t *uisb, double pos);
void uiScrollbarSetStepIncrement (uiwidget_t *uisb, double step);
void uiScrollbarSetPageIncrement (uiwidget_t *uisb, double page);
void uiScrollbarSetPageSize (uiwidget_t *uisb, double sz);

#endif /* INC_UISCROLLBAR_H */
