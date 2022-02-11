#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdjstring.h"
#include "portability.h"

double
dRandom (void)
{
  double      dval;

#if _lib_drand48
  dval = drand48 ();
#endif
#if ! _lib_drand48 && _lib_random
  long    lval;

  lval = random ();
  dval = (double) ival / (double) LONG_MAX;
#endif
#if ! _lib_drand48 && ! _lib_random && _lib_rand
  int       ival;

  ival = rand ();
  dval = (double) ival / (double) RAND_MAX;
#endif
  return dval;
}

void
sRandom (void)
{
  pid_t pid = getpid ();
  long  seed = (ssize_t) time (NULL) ^ (pid + (pid << 15));
#if _lib_srand48
  srand48 (seed);
#endif
#if ! _lib_srand48 && _lib_random
  srandom ((unsigned int) seed);
#endif
#if ! _lib_srand48 && ! _lib_random && _lib_srand
  srand ((unsigned int) seed);
#endif
}

char *
getHostname (char *buff, size_t sz)
{
  *buff = '\0';
  gethostname (buff, sz);

  if (*buff == '\0') {
    /* gethostname() does not appear to work on windows */
    char *hn = getenv ("COMPUTERNAME");
    if (hn != NULL) {
      strlcpy (buff, hn, sz);
      stringToLower (buff);
    }
  }

  return buff;
}
