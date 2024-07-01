/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGENRE_H
#define INC_UIGENRE_H

#include "callback.h"
#include "ilist.h"
#include "uiwcont.h"

typedef struct uigenre uigenre_t;

uigenre_t * uigenreDropDownCreate (uiwcont_t *boxp, uiwcont_t *parentwin, bool allflag);
uiwcont_t * uigenreGetButton (uigenre_t *uigenre);
void uigenreFree (uigenre_t *uigenre);
ilistidx_t uigenreGetKey (uigenre_t *uigenre);
void uigenreSetKey (uigenre_t *uigenre, ilistidx_t gkey);
void uigenreSetState (uigenre_t *uigenre, int state);
void uigenreSetCallback (uigenre_t *uigenre, callback_t *cb);

#endif /* INC_UIGENRE_H */
