#ifndef INC_SYSVARS_H
#define INC_SYSVARS_H

#include "portability.h"

typedef enum {
  SV_OSNAME,
  SV_OSVERS,
  SV_OSDISP,
  SV_HOSTNAME,
  SV_MAX
} sysvarkey_t;

typedef enum {
  SVL_BDJIDX,
  SVL_MAX
} sysvarlkey_t;

extern char       sysvars [SV_MAX][MAXPATHLEN];
extern long       lsysvars [SVL_MAX];

void    sysvarsInit (void);
void    sysvarSetLong (sysvarlkey_t, long);
int     isMacOS (void);
int     isWindows (void);
int     isLinux (void);

#endif /* INC_SYSVARS_H */
