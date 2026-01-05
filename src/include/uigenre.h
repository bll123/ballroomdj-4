/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "ilist.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uigenre uigenre_t;

uigenre_t * uigenreDropDownCreate (uiwcont_t *boxp, uiwcont_t *parentwin, bool allflag);
uiwcont_t * uigenreGetButton (uigenre_t *uigenre);
void uigenreFree (uigenre_t *uigenre);
ilistidx_t uigenreGetKey (uigenre_t *uigenre);
void uigenreSetKey (uigenre_t *uigenre, ilistidx_t gkey);
void uigenreSetState (uigenre_t *uigenre, int state);
void uigenreSetCallback (uigenre_t *uigenre, callback_t *cb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

