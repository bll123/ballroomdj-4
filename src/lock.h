#ifndef INC_LOCK_H
#define INC_LOCK_H

#include <sys/types.h>

#define MAIN_LOCK_FN        "main"
#define MOBILEMQ_LOCK_FN    "mobilemq"
#define REMCTRL_LOCK_FN     "remctrl"
#define PLAYER_LOCK_FN      "player"
#define MARQUEE_LOCK_FN     "marquee"
#define PLAYERUI_LOCK_FN    "playerui"

pid_t lockExists (char *, int);
int   lockAcquire (char *, int);
int   lockAcquirePid (char *, pid_t, int);
int   lockRelease (char *, int);
int   lockReleasePid (char *, pid_t, int);

#endif /* INC_LOCK_H */
