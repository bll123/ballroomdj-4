/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIADJUSTMENT_H
#define INC_UIADJUSTMENT_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "uiwcont.h"

uiwcont_t *uiCreateAdjustment (double value, double start, double end, double stepinc, double pageinc, double pagesz);
void * uiAdjustmentGetAdjustment (uiwcont_t *uiadj);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIADJUSTMENT_H */
