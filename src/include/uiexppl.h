/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIEXPPL_H
#define INC_UIEXPPL_H

#include "nlist.h"
#include "ui.h"

typedef struct uiexppl uiexppl_t;

uiexppl_t  *uiexpplInit (uiwcont_t *windowp, nlist_t *opts);
void    uiexpplFree (uiexppl_t *uiexppl);
void    uiexpplSetResponseCallback (uiexppl_t *uiexppl, callback_t *uicb);
bool    uiexpplDialog (uiexppl_t *uiexppl);
void    uiexpplDialogClear (uiexppl_t *uiexppl);
void    uiexpplProcess (uiexppl_t *uiexppl);
char    *uiexpplGetDir (uiexppl_t *uiexppl);
const char  *uiexpplGetNewName (uiexppl_t *uiexppl);
void uiexpplUpdateStatus (uiexppl_t *uiexppl, int count, int tot);

#endif /* INC_UIEXPPL_H */
