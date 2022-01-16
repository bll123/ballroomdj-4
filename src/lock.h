#ifndef INC_LOCK_H
#define INC_LOCK_H

#include <sys/types.h>

#include "datautil.h"

#define MOBILEMQ_LOCK_FN "mobilemq"

int   lockExists (char *, datautil_mp_t);
int   lockAcquire (char *, datautil_mp_t);
int   lockAcquirePid (char *, pid_t, datautil_mp_t);
int   lockRelease (char *, datautil_mp_t);
int   lockReleasePid (char *, pid_t, datautil_mp_t);

#endif /* INC_LOCK_H */
