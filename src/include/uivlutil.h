/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIVLUTIL_H
#define INC_UIVLUTIL_H

#include "slist.h"
#include "uivirtlist.h"

void uivlAddDisplayColumns (uivirtlist_t *uivl, slist_t *sellist, int startcol);
void uivlAddFavoriteClasses (void);

#endif /* INC_UIVLUTIL_H */
