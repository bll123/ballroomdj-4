#ifndef INC_PORTABILITY_H
#define INC_PORTABILITY_H

#include <limits.h>
#include <sys/param.h>
#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

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

double  dRandom (void);
void    sRandom (void);

#endif /* INC_PORTABILITY_H */
