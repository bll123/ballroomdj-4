/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_STARTUTIL_H
#define INC_STARTUTIL_H

#include "conn.h"

void starterStopAllProcesses (conn_t *conn);
void starterRemoveAllLocks (void);
void starterCleanVolumeReg (void);
void starterPlayerStartup (void);
void starterPlayerShutdown (void);

#endif /* INC_STARTUTIL_H */
