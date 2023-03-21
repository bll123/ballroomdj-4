/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISCROLLBAR_H
#define INC_UISCROLLBAR_H

#include "callback.h"
#include "uiwcont.h"

typedef struct uiscrollbar uiscrollbar_t;

uiscrollbar_t *uiCreateVerticalScrollbar (double upper);
void uiScrollbarFree (uiscrollbar_t *sb);
uiwcont_t * uiScrollbarGetWidgetContainer (uiscrollbar_t *sb);
void uiScrollbarSetChangeCallback (uiscrollbar_t *sb, callback_t *cb);
void uiScrollbarSetUpper (uiscrollbar_t *sb, double upper);
void uiScrollbarSetPosition (uiscrollbar_t *sb, double pos);
void uiScrollbarSetStepIncrement (uiscrollbar_t *sb, double step);
void uiScrollbarSetPageIncrement (uiscrollbar_t *sb, double page);
void uiScrollbarSetPageSize (uiscrollbar_t *sb, double sz);

#endif /* INC_UISCROLLBAR_H */
