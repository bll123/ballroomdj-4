#ifndef INC_SYSVARS_H
#define INC_SYSVARS_H

#include "portability.h"

typedef enum {
  SV_OSNAME,
  SV_OSVERS,
  SV_OSBUILD,
  SV_OSDISP,
  SV_HOSTNAME,
  SV_BDJ4DATATOPDIR,  // path to the directory above data/ and tmp/ and http/
  SV_BDJ4MAINDIR,     // path to the main directory above bin/, etc.
  SV_BDJ4DATADIR,     // data
  SV_BDJ4TMPDIR,      // tmp
  SV_BDJ4EXECDIR,     // main + /bin
  SV_BDJ4IMGDIR,      // main + /img
  SV_BDJ4LOCALEDIR,   // main + /locale
  SV_BDJ4TEMPLATEDIR, // main + /templates
  SV_BDJ4HTTPDIR,     // http
  SV_SHLIB_EXT,
  SV_MOBMQ_HOST,
  SV_LOCALE,
  SV_CA_FILE,
  SV_BDJ4_VERSION,
  SV_BDJ4_BUILD,
  SV_BDJ4_RELEASELEVEL,
  SV_MAX
} sysvarkey_t;

typedef enum {
  SVL_BDJIDX,
  SVL_BASEPORT,
  SVL_OSBITS,
  SVL_MAX
} sysvarlkey_t;

void    sysvarsInit (const char *);
char    * sysvarsGetStr (sysvarkey_t idx);
ssize_t sysvarsGetNum (sysvarlkey_t idx);
void    sysvarSetNum (sysvarlkey_t, ssize_t);
bool    isMacOS (void);
bool    isWindows (void);
bool    isLinux (void);

#endif /* INC_SYSVARS_H */
