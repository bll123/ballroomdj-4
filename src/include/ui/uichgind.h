/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UICHGIND_H
#define INC_UICHGIND_H

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateChangeIndicator (uiwcont_t *boxp);
void  uichgindMarkNormal (uiwcont_t *uiwidget);
void  uichgindMarkChanged (uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIWIDGET_H */
