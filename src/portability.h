#ifndef INC_PORTABILITY_H
#define INC_PORTABILITY_H

#include "config.h"

#include <limits.h>
#include <sys/param.h>
#if _hdr_windows
# include <windows.h>
#endif

#define PID_FMT "%d"
#if _siz_pid_t == 8 && _siz_long == 8
# undef PID_FMT
# define PID_FMT "%ld"
#endif
#if _siz_pid_t == 8 && _siz_long == 4
# undef PID_FMT
# define PID_FMT "%lld"
#endif

#define SIZE_FMT "%lu"
#if _siz_size_t == 8 && _siz_long == 4
# undef SIZE_FMT
# define SIZE_FMT "%llu"
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

#endif /* INC_PORTABILITY_H */
