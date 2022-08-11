#ifndef INC_OSPROCESS_H
#define INC_OSPROCESS_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "config.h"

#include <sys/types.h>

enum {
  OS_PROC_NONE    = 0x0000,
  OS_PROC_DETACH  = 0x0001,
  OS_PROC_WAIT    = 0x0002,
};

pid_t osProcessStart (const char *targv[], int flags, void **handle, char *outfname);
pid_t osProcessPipe (const char *targv[], int flags, char *rbuff, size_t sz);
char *osRunProgram (const char *prog, ...);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INC_OSPROCESS_H */
