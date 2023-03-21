/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISEP_H
#define INC_UISEP_H

#include "uiwcont.h"

uiwcont_t *uiCreateHorizSeparator (void);
void uiSeparatorAddClass (const char *classnm, const char *color);

#endif /* INC_UISEP_H */
