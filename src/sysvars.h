#ifndef _INC_SYSVARS_H
#define _INC_SYSVARS_H

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
  SV_HOSTNAME,
  SV_MAX
} sysvarkey_t;

typedef struct {
    char      *name;
    char      value [MAXPATHLEN+1];
} sysvar_t;

extern sysvar_t sysvars [SV_MAX];

void sysvarsInit (void);

#endif /* _INC_SYSVARS_H */
