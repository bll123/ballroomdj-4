#ifndef INC_LOCK_H
#define INC_LOCK_H

#include <sys/types.h>

int   lockAcquire (char *);
int   lockAcquirePid (char *, pid_t);
int   lockRelease (char *);
int   lockReleasePid (char *, pid_t);

#endif /* INC_LOCK_H */
