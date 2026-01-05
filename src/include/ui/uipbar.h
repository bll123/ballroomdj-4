/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateProgressBar (void);
void uiProgressBarSet (uiwcont_t *uipb, double val);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

