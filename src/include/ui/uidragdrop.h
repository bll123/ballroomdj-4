/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIDRAGDROP_H
#define INC_UIDRAGDROP_H

#include "callback.h"
#include "uiwcont.h"

void uiDragDropSetDestURICallback (uiwcont_t *uiwcont, callback_t *cb);
void uiDragDropSetDestDataCallback (uiwcont_t *uiwcont, callback_t *cb);
void uiDragDropSetSourceDataCallback (uiwcont_t *uiwcont, callback_t *cb);

#endif /* INC_UIDRAGDROP_H */
