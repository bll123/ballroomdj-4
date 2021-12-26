#ifndef INC_PORTABILITY_H
#define INC_PORTABILITY_H

#include "config.h"
#if _hdr_windows
# include <windows.h>
#endif
#include "portability.h"

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

#endif /* INC_PORTABILITY_H */
