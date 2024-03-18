/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UILINK_H
#define INC_UILINK_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "callback.h"
#include "uiwcont.h"

uiwcont_t *uiCreateLink (const char *label, const char *uri);
void uiLinkSet (uiwcont_t *uilink, const char *label, const char *uri);
void uiLinkSetActivateCallback (uiwcont_t *uilink, callback_t *uicb);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UILINK_H */
