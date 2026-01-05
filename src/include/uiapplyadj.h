/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "nlist.h"
#include "song.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uiaa uiaa_t;

uiaa_t  *uiaaInit (uiwcont_t *windowp, nlist_t *opts);
void    uiaaFree (uiaa_t *uiaa);
void    uiaaSetResponseCallback (uiaa_t *uiaa, callback_t *uicb);
bool    uiaaDialog (uiaa_t *uiaa, int aaflags, bool hasorig);
void    uiaaDialogClear (uiaa_t *uiaa);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

