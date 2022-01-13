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
#include <limits.h>
#if _hdr_dlfcn
# include <dlfcn.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

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
  size_t seed = time(NULL) ^ (pid + (pid << 15));
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

dlhandle_t *
dylibLoad (char *path)
{
  void      *handle;

#if _lib_dlopen
  handle = dlopen (path, RTLD_LAZY);
#endif
#if _lib_LoadLibrary
  handle = LoadLibrary (path);
#endif
  if (handle == NULL) {
    fprintf (stderr, "dlopen %s failed: %d %s\n", path, errno, strerror (errno));
  }
  return handle;
}

void
dylibClose (dlhandle_t *handle)
{
#if _lib_dlopen
  dlclose (handle);
#endif
#if _lib_LoadLibrary
  FreeLibrary (handle);
#endif
}

void *
dylibLookup (dlhandle_t *handle, char *funcname)
{
  void      *addr;

#if _lib_dlopen
  addr = dlsym (handle, funcname);
#endif
#if _lib_LoadLibrary
  addr = GetProcAddress (handle, funcname);
#endif
  if (addr == NULL) {
    fprintf (stderr, "sym lookup %s failed: %d %s\n", funcname, errno, strerror (errno));
  }
  return addr;
}
