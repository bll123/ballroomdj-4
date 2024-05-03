/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIEXP_H
#define INC_UIEXP_H

#include "nlist.h"
#include "ui.h"

typedef struct uiexp uiexp_t;

uiexp_t  *uiexpInit (uiwcont_t *windowp, nlist_t *opts);
void    uiexpFree (uiexp_t *uiexp);
void    uiexpSetResponseCallback (uiexp_t *uiexp, callback_t *uicb);
bool    uiexpDialog (uiexp_t *uiexp);
void    uiexpDialogClear (uiexp_t *uiexp);
void    uiexpProcess (uiexp_t *uiexp);
char    *uiexpGetDir (uiexp_t *uiexp);
const char  *uiexpGetNewName (uiexp_t *uiexp);
void uiexpUpdateStatus (uiexp_t *uiexp, int count, int tot);

#endif /* INC_UIEXP_H */
