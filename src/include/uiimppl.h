/*
 * Copyright 2025-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "nlist.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uiimppl uiimppl_t;

uiimppl_t  *uiimpplInit (uiwcont_t *windowp, nlist_t *opts, const char *waitmsg);
void    uiimpplFree (uiimppl_t *uiimppl);
void    uiimpplSetResponseCallback (uiimppl_t *uiimppl, callback_t *uicb);
bool    uiimpplDialog (uiimppl_t *uiimppl, const char *uri);
void    uiimpplProcess (uiimppl_t *uiimppl);
void    uiimpplDialogClear (uiimppl_t *uiimppl);
int     uiimpplGetType (uiimppl_t *uiimppl);
int     uiimpplGetASKey (uiimppl_t *uiimppl);
const char *uiimpplGetURI (uiimppl_t *uiimppl);
const char *uiimpplGetOrigName (uiimppl_t *uiimppl);
const char *uiimpplGetNewName (uiimppl_t *uiimppl);
void uiimpplUpdateStatus (uiimppl_t *uiimppl, int count, int tot);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

