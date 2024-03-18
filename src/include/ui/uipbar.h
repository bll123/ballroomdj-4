/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIPBAR_H
#define INC_UIPBAR_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "uiwcont.h"

uiwcont_t *uiCreateProgressBar (void);
void uiProgressBarSet (uiwcont_t *uipb, double val);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIPBAR_H */
