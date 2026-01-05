/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "config.h"
#include "nodiscard.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct dirhandle dirhandle_t;

NODISCARD dirhandle_t   * osDirOpen (const char *dir);
char          * osDirIterate (dirhandle_t *dirh);
void          osDirClose (dirhandle_t *dirh);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

