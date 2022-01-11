#ifndef INC_LOCK_H
#define INC_LOCK_H

#include <sys/types.h>

#include "datautil.h"

int   lockAcquire (char *, datautil_mp_t);
int   lockAcquirePid (char *, pid_t, datautil_mp_t);
int   lockRelease (char *, datautil_mp_t);
int   lockReleasePid (char *, pid_t, datautil_mp_t);

#endif /* INC_LOCK_H */
