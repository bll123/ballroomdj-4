/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "config.h"

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  OS_PROC_NONE        = 0,
  OS_PROC_DETACH      = (1 << 0),
  OS_PROC_WAIT        = (1 << 1),
  OS_PROC_NOSTDERR    = (1 << 2),
  OS_PROC_NOWINDOW    = (1 << 3),
  OS_PROC_WINDOW_OK   = (1 << 4),
};

pid_t osProcessStart (const char *targv[], int flags, void **handle, char *outfname);
int osProcessPipe (const char *targv[], int flags, char *rbuff, size_t sz, size_t *retsz);
char *osRunProgram (const char *prog, ...);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

