/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PROCUTIL_H
#define INC_PROCUTIL_H

#include "config.h"

#include <stdbool.h>
#include <sys/types.h>

#include "bdjmsg.h"
#include "conn.h"
#include "log.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  PROCUTIL_NO_DETACH,
  PROCUTIL_DETACH,
};

enum {
  PROCUTIL_NORM_TERM = false,
  PROCUTIL_FORCE_TERM = true,
};

typedef struct {
  void      *processHandle;
  pid_t     pid;
  bool      started : 1;
  bool      hasHandle : 1;
} procutil_t;

void        procutilInitProcesses (procutil_t *processes [ROUTE_MAX]);
int         procutilExists (procutil_t *process);
procutil_t  * procutilStart (const char *fn, int profile, loglevel_t loglvl,
    int detachflag, const char *aargs []);
int         procutilKill (procutil_t *process, bool force);
void        procutilTerminate (pid_t pid, bool force);
void        procutilFreeAll (procutil_t *processes [ROUTE_MAX]);
void        procutilFreeRoute (procutil_t *processes [ROUTE_MAX], bdjmsgroute_t route);
void        procutilFree (procutil_t *process);
procutil_t  * procutilStartProcess (bdjmsgroute_t route, const char *fname,
    int detachflag, const char *aargs []);
void        procutilStopAllProcess (procutil_t *processes [ROUTE_MAX],
    conn_t *conn, bool force);
void        procutilStopProcess (procutil_t *process, conn_t *conn,
    bdjmsgroute_t route, bool force);
void        procutilCloseProcess (procutil_t *process, conn_t *conn,
    bdjmsgroute_t route);
void        procutilForceStop (procutil_t *process, int flags,
    bdjmsgroute_t route);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_PROCUTIL_H */
