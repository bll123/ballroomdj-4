/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UICOPYTAGS_H
#define INC_UICOPYTAGS_H

#include "nlist.h"
#include "ui.h"

typedef struct uict uict_t;

uict_t  *uicopytagsInit (uiwcont_t *windowp, nlist_t *opts);
void    uicopytagsFree (uict_t *uict);
bool    uicopytagsDialog (uict_t *uict);
void    uicopytagsProcess (uict_t *uict);

#endif /* INC_UICOPYTAGS_H */
