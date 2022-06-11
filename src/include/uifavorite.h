#ifndef INC_UIFAVORITE_H
#define INC_UIFAVORITE_H

#include "uiutils.h"

typedef struct uifavorite uifavorite_t;

uifavorite_t * uifavoriteSpinboxCreate (UIWidget *boxp);
void uifavoriteFree (uifavorite_t *uifavorite);
int uifavoriteGetValue (uifavorite_t *uifavorite);
void uifavoriteSetValue (uifavorite_t *uifavorite, int value);
void uifavoriteDisable (uifavorite_t *uifavorite);
void uifavoriteEnable (uifavorite_t *uifavorite);

#endif /* INC_UIFAVORITE_H */
