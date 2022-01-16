#ifndef INC_SYSVARS_H
#define INC_SYSVARS_H

#include "config.h"

#include "portability.h"

typedef enum {
  SV_OSNAME,
  SV_OSVERS,
  SV_OSBUILD,
  SV_OSDISP,
  SV_HOSTNAME,
  SV_BDJ4EXECDIR,     // where the executables are
  SV_BDJ4DIR,         // where the data directory is
  SV_SHLIB_EXT,
  SV_MOBMQ_HOST,
  SV_CA_FILE,
  SV_BDJ4_VERSION,
  SV_BDJ4_BUILD,
  SV_MAX
} sysvarkey_t;

typedef enum {
  SVL_BDJIDX,
  SVL_MAX
} sysvarlkey_t;

extern char       sysvars [SV_MAX][MAXPATHLEN];
extern ssize_t    lsysvars [SVL_MAX];

void    sysvarsInit (const char *);
void    sysvarSetNum (sysvarlkey_t, ssize_t);
bool    isMacOS (void);
bool    isWindows (void);
bool    isLinux (void);

#endif /* INC_SYSVARS_H */
