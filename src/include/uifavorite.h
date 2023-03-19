/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIFAVORITE_H
#define INC_UIFAVORITE_H

#include "callback.h"
#include "musicdb.h"
#include "ui.h"

typedef struct uifavorite uifavorite_t;

uifavorite_t * uifavoriteSpinboxCreate (uiwidget_t *boxp);
void uifavoriteFree (uifavorite_t *uifavorite);
int uifavoriteGetValue (uifavorite_t *uifavorite);
void uifavoriteSetValue (uifavorite_t *uifavorite, int value);
void uifavoriteSetState (uifavorite_t *uifavorite, int state);
void uifavoriteSetChangedCallback (uifavorite_t *uifavorite, callback_t *cb);

#endif /* INC_UIFAVORITE_H */
