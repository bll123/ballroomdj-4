/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "slist.h"
#include "uivirtlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void uivlAddDisplayColumns (uivirtlist_t *uivl, slist_t *sellist, int startcol);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

