#ifndef INC_OSUTILS_H
#define INC_OSUTILS_H

#include <sys/types.h>

enum {
  OS_PROC_NONE    = 0x0000,
  OS_PROC_DETACH  = 0x0001,
};

double        dRandom (void);
void          sRandom (void);
pid_t         osProcessStart (char *targv[], int flags, void **handle);

#endif /* INC_OSUTILS_H */
