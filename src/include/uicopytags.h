/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nlist.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uict uict_t;

uict_t  *uicopytagsInit (uiwcont_t *windowp, nlist_t *opts);
void    uicopytagsFree (uict_t *uict);
bool    uicopytagsDialog (uict_t *uict);
void    uicopytagsProcess (uict_t *uict);
int     uicopytagsState (uict_t *uict);
void    uicopytagsReset (uict_t *uict);
const char * uicopytagsGetFilename (uict_t *uict);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

