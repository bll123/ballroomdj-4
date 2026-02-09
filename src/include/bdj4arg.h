/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct bdj4arg bdj4arg_t;

BDJ_NODISCARD bdj4arg_t *bdj4argInit (int argc, char *argv []);
void bdj4argCleanup (bdj4arg_t *bdj4arg);
char **bdj4argGetArgv (bdj4arg_t *bdj4arg);
const char * bdj4argGet (bdj4arg_t *bdj4arg, int idx, const char *arg);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

