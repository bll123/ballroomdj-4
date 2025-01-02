/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_STARTUTIL_H
#define INC_STARTUTIL_H

#include "conn.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void starterStopAllProcesses (conn_t *conn);
void starterRemoveAllLocks (void);
void starterCleanVolumeReg (void);
void starterPlayerStartup (void);
void starterPlayerShutdown (void);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_STARTUTIL_H */
