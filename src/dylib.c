#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#if _hdr_dlfcn
# include <dlfcn.h>
#endif
#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "dylib.h"
#include "pathutil.h"
#include "portability.h"

dlhandle_t *
dylibLoad (char *path)
{
  void      *handle = NULL;

#if _lib_dlopen
  handle = dlopen (path, RTLD_LAZY);
#endif
#if _lib_LoadLibrary
  HMODULE   whandle;
  char      npath [MAXPATHLEN];

  pathToWinPath (path, npath, MAXPATHLEN);
  whandle = LoadLibrary (npath);
  handle = whandle;
#endif
  if (handle == NULL) {
    fprintf (stderr, "dylib open %s failed: %d %s\n", path, errno, strerror (errno));
  } else {
//    fprintf (stderr, "dylib open %s OK\n", path);
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
  HMODULE   whandle = handle;
  FreeLibrary (whandle);
#endif
}

void *
dylibLookup (dlhandle_t *handle, char *funcname)
{
  void      *addr = NULL;

#if _lib_dlopen
  addr = dlsym (handle, funcname);
#endif
#if _lib_LoadLibrary
  HMODULE   whandle = handle;
  addr = GetProcAddress (whandle, funcname);
#endif
  if (addr == NULL) {
    fprintf (stderr, "sym lookup %s failed: %d %s\n", funcname, errno, strerror (errno));
#if _lib_LoadLibrary
    fprintf (stderr, "sym lookup %s getlasterror: %ld\n", funcname, GetLastError() );
#endif
  } else {
//    fprintf (stderr, "sym lookup %s OK\n", funcname);
  }
  return addr;
}
