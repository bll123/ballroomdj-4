#ifndef _INC_LOCK_H
#define _INC_LOCK_H

#include <sys/types.h>

int   lockAcquire (char *);
int   lockAcquirePid (char *, pid_t);
int   lockRelease (char *);
int   lockReleasePid (char *, pid_t);

#endif /* _INC_LOCK_H */
