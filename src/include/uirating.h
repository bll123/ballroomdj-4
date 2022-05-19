#ifndef INC_UIRATING_H
#define INC_UIRATING_H

#include "uiutils.h"

typedef struct uirating uirating_t;

uirating_t * uiratingSpinboxCreate (UIWidget *boxp, bool allflag);
void uiratingFree (uirating_t *uirating);
int uiratingGetValue (uirating_t *uirating);
void uiratingSetValue (uirating_t *uirating, int value);

#endif /* INC_UIRATING_H */
