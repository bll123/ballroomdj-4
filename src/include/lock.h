/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <sys/types.h>

#include "bdjmsg.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

char  * lockName (bdjmsgroute_t route);
pid_t lockExists (char *, int);
int   lockAcquire (char *, int);
int   lockRelease (char *, int);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

