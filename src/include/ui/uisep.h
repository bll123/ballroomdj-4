/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISEP_H
#define INC_UISEP_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "uiwcont.h"

uiwcont_t *uiCreateHorizSeparator (void);
void uiSeparatorAddClass (const char *classnm, const char *color);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UISEP_H */
