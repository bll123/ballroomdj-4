/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISCROLLBAR_H
#define INC_UISCROLLBAR_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void uiCreateVerticalScrollbar (UIWidget *uiwidget, double upper);
void uiScrollbarSetUpper (UIWidget *uisb, double upper);
void uiScrollbarSetPosition (UIWidget *uisb, double pos);
void uiScrollbarSetStepIncrement (UIWidget *uisb, double step);
void uiScrollbarSetPageIncrement (UIWidget *uisb, double page);
void uiScrollbarSetPageSize (UIWidget *uisb, double sz);

#endif /* INC_UISCROLLBAR_H */
