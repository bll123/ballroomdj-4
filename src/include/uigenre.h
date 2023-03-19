/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGENRE_H
#define INC_UIGENRE_H

#include "ui.h"

typedef struct uigenre uigenre_t;

uigenre_t * uigenreDropDownCreate (UIWidget *boxp, uiwidget_t *parentwin, bool allflag);
UIWidget * uigenreGetButton (uigenre_t *uigenre);
void uigenreFree (uigenre_t *uigenre);
int uigenreGetValue (uigenre_t *uigenre);
void uigenreSetValue (uigenre_t *uigenre, int value);
void uigenreSetState (uigenre_t *uigenre, int state);
void uigenreSizeGroupAdd (uigenre_t *uigenre, uiwidget_t *sg);
void uigenreSetCallback (uigenre_t *uigenre, callback_t *cb);

#endif /* INC_UIGENRE_H */
