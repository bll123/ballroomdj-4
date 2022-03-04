#ifndef INC_procutil_H
#define INC_procutil_H

#include "config.h"

#include <stdbool.h>
#include <sys/types.h>

#include "bdjmsg.h"
#include "conn.h"

enum {
  PROCUTIL_NO_DETACH,
  PROCUTIL_DETACH,
};

typedef struct {
  void      *processHandle;
  pid_t     pid;
  bool      started : 1;
  bool      hasHandle : 1;
} procutil_t;

int         procutilExists (procutil_t *process);
procutil_t  * procutilStart (const char *fn, ssize_t profile, ssize_t loglvl,
                int detachflag);
int         procutilKill (procutil_t *process, bool force);
void        procutilTerminate (pid_t pid, bool force);
void        procutilFree (procutil_t *process);
void        procutilCatchSignal (void (*sigHandler)(int), int signal);
void        procutilIgnoreSignal (int signal);
void        procutilDefaultSignal (int signal);
procutil_t  * procutilStartProcess (bdjmsgroute_t route, char *fname, int detachflag);
void        procutilStopProcess (procutil_t *process, conn_t *conn,
                bdjmsgroute_t route, bool force);
void        procutilForceStop (procutil_t *process, int flags,
                bdjmsgroute_t route);

#endif /* INC_procutil_H */
