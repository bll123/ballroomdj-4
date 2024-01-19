/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIAPPLYADJ_H
#define INC_UIAPPLYADJ_H

#include "nlist.h"
#include "song.h"
#include "ui.h"

typedef struct uiaa uiaa_t;

uiaa_t  *uiaaInit (uiwcont_t *windowp, nlist_t *opts);
void    uiaaFree (uiaa_t *uiaa);
void    uiaaSetResponseCallback (uiaa_t *uiaa, callback_t *uicb);
bool    uiaaDialog (uiaa_t *uiaa, int aaflags, bool hasorig);
void    uiaaDialogClear (uiaa_t *uiaa);

#endif /* INC_UIAPPLYADJ_H */
