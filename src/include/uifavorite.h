/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIFAVORITE_H
#define INC_UIFAVORITE_H

#include "callback.h"
#include "musicdb.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uifavorite uifavorite_t;

uifavorite_t * uifavoriteSpinboxCreate (uiwcont_t *boxp);
void uifavoriteFree (uifavorite_t *uifavorite);
int uifavoriteGetValue (uifavorite_t *uifavorite);
void uifavoriteSetValue (uifavorite_t *uifavorite, int value);
void uifavoriteSetState (uifavorite_t *uifavorite, int state);
void uifavoriteSetChangedCallback (uifavorite_t *uifavorite, callback_t *cb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIFAVORITE_H */
