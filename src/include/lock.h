#ifndef INC_LOCK_H
#define INC_LOCK_H

#include <sys/types.h>

#include "bdjmsg.h"

char  * lockName (bdjmsgroute_t route);
pid_t lockExists (char *, int);
int   lockAcquire (char *, int);
int   lockAcquirePid (char *, pid_t, int);
int   lockRelease (char *, int);
int   lockReleasePid (char *, pid_t, int);

#endif /* INC_LOCK_H */
