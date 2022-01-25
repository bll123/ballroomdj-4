#ifndef INC_PROCESS_H
#define INC_PROCESS_H

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

typedef struct {
  bool      hasHandle;
#if _typ_HANDLE
  HANDLE    processHandle;
#endif
  pid_t     pid;
} process_t;

int         processExists (process_t *process);
process_t   *processStart (const char *fn, ssize_t profile, ssize_t loglvl);
int         processKill (process_t *process);
void        processFree (process_t *process);
#if _typ_HANDLE
HANDLE      processGetProcessHandle (pid_t pid);
#endif
void        processCatchSignal (void (*sigHandler)(int), int signal);
void        processIgnoreSignal (int signal);
void        processDefaultSignal (int signal);

#endif /* INC_PROCESS_H */
