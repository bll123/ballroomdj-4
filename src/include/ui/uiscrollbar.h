/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISCROLLBAR_H
#define INC_UISCROLLBAR_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void uiCreateVerticalScrollbar (uiwcont_t *uiwidget, double upper);
void uiScrollbarSetUpper (uiwcont_t *uisb, double upper);
void uiScrollbarSetPosition (uiwcont_t *uisb, double pos);
void uiScrollbarSetStepIncrement (uiwcont_t *uisb, double step);
void uiScrollbarSetPageIncrement (uiwcont_t *uisb, double page);
void uiScrollbarSetPageSize (uiwcont_t *uisb, double sz);

#endif /* INC_UISCROLLBAR_H */
