#ifndef INC_PROCESS_H
#define INC_PROCESS_H

#include <stdbool.h>
#include <sys/types.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdjmsg.h"
#include "conn.h"

typedef struct {
#if _typ_HANDLE
  HANDLE    processHandle;
#endif
  pid_t     pid;
  bool      started : 1;
  bool      hasHandle : 1;
} process_t;

int         processExists (process_t *process);
process_t   *processStart (const char *fn, ssize_t profile, ssize_t loglvl);
int         processKill (process_t *process, bool force);
void        processTerminate (pid_t pid, bool force);
void        processFree (process_t *process);
void        processCatchSignal (void (*sigHandler)(int), int signal);
void        processIgnoreSignal (int signal);
void        processDefaultSignal (int signal);
process_t   *processStartProcess (bdjmsgroute_t route, char *fname);
void        processStopProcess (process_t *process, conn_t *conn,
                bdjmsgroute_t route, bool force);
void        processForceStop (process_t *process, int flags,
                bdjmsgroute_t route);

#endif /* INC_PROCESS_H */
