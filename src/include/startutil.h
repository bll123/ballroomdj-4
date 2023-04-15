#ifndef INC_STARTUTIL_H
#define INC_STARTUTIL_H

#include "conn.h"

void starterStopAllProcesses (conn_t *conn);
void starterRemoveAllLocks (void);
void starterCleanVolumeReg (void);

#endif /* INC_STARTUTIL_H */
