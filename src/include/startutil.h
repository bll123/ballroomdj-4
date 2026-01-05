/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "conn.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void starterStopAllProcesses (conn_t *conn);
void starterRemoveAllLocks (void);
void starterCleanVolumeReg (void);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

