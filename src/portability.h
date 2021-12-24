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

#endif /* INC_PORTABILITY_H */
