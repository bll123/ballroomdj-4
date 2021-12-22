#ifndef INC_SYSVARS_H
#define INC_SYSVARS_H

#include <limits.h>
#include <sys/param.h>

#if ! defined (MAXPATHLEN)
# if defined (_POSIX_PATH_MAX)
#  define MAXPATHLEN        _POSIX_PATH_MAX
# else
#  if defined (PATH_MAX)
#   define MAXPATHLEN       PATH_MAX
#  endif
#  if defined (LPNMAX)
#   define MAXPATHLEN       LPNMAX
#  endif
# endif
#endif

#if ! defined (MAXPATHLEN)
# define MAXPATHLEN         255
#endif

typedef enum {
  SV_OSNAME,
  SV_OSVERS,
  SV_OSDISP,
  SV_HOSTNAME,
  SV_MAX
} sysvarkey_t;

extern char       sysvars [SV_MAX][MAXPATHLEN];

void sysvarsInit (void);
int isMacOS (void);
int isWindows (void);
int isLinux (void);

#endif /* INC_SYSVARS_H */
