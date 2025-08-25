/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateHorizSeparator (void);
void uiSeparatorAddClass (const char *classnm, const char *color);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

