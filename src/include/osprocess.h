/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_OSPROCESS_H
#define INC_OSPROCESS_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "config.h"

#include <sys/types.h>

enum {
  OS_PROC_NONE      = 0,
  OS_PROC_DETACH    = (1 << 0),
  OS_PROC_WAIT      = (1 << 1),
  OS_PROC_NOSTDERR  = (1 << 2),
  OS_PROC_WINDOW_OK = (1 << 3), // windows, default is no window
};

pid_t osProcessStart (const char *targv[], int flags, void **handle, char *outfname);
int osProcessPipe (const char *targv[], int flags, char *rbuff, size_t sz, size_t *retsz);
char *osRunProgram (const char *prog, ...);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INC_OSPROCESS_H */
