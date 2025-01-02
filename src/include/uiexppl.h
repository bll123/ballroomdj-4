/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIEXPPL_H
#define INC_UIEXPPL_H

#include "callback.h"
#include "nlist.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* import/export types */
enum {
  EI_TYPE_M3U,
  EI_TYPE_XSPF,
  EI_TYPE_JSPF,
  EI_TYPE_MAX,
};

typedef struct uiexppl uiexppl_t;

uiexppl_t  *uiexpplInit (uiwcont_t *windowp, nlist_t *opts);
void    uiexpplFree (uiexppl_t *uiexppl);
void    uiexpplSetResponseCallback (uiexppl_t *uiexppl, callback_t *uicb);
bool    uiexpplDialog (uiexppl_t *uiexppl, const char *slname);
void    uiexpplProcess (uiexppl_t *uiexppl);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIEXPPL_H */
