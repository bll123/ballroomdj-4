/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISIZEGRP_H
#define INC_UISIZEGRP_H

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateSizeGroupHoriz (void);
void uiSizeGroupAdd (uiwcont_t *uisg, uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UISIZEGRP_H */
