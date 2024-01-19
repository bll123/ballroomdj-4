/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISCROLLBAR_H
#define INC_UISCROLLBAR_H

#include "callback.h"
#include "uiwcont.h"

uiwcont_t *uiCreateVerticalScrollbar (double upper);
void uiScrollbarFree (uiwcont_t *sb);
void uiScrollbarSetChangeCallback (uiwcont_t *sb, callback_t *cb);
void uiScrollbarSetUpper (uiwcont_t *sb, double upper);
void uiScrollbarSetPosition (uiwcont_t *sb, double pos);
void uiScrollbarSetStepIncrement (uiwcont_t *sb, double step);
void uiScrollbarSetPageIncrement (uiwcont_t *sb, double page);
void uiScrollbarSetPageSize (uiwcont_t *sb, double sz);

#endif /* INC_UISCROLLBAR_H */
