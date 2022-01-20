#ifndef INC_LOCK_H
#define INC_LOCK_H

#include <sys/types.h>

#include "pathbld.h"

#define MAIN_LOCK_FN        "main"
#define MOBILEMQ_LOCK_FN    "mobilemq"
#define REMCTRL_LOCK_FN     "remctrl"
#define PLAYER_LOCK_FN      "player"

int   lockExists (char *, datautil_mp_t);
int   lockAcquire (char *, datautil_mp_t);
int   lockAcquirePid (char *, pid_t, datautil_mp_t);
int   lockRelease (char *, datautil_mp_t);
int   lockReleasePid (char *, pid_t, datautil_mp_t);

#endif /* INC_LOCK_H */
