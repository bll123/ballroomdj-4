/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#if __has_include (<dlfcn.h>)
# include <dlfcn.h>
#endif

#if __has_include (<windows.h>)
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "dylib.h"
#include "mdebug.h"
#include "osutils.h"
#include "pathdisp.h"
#include "sysvars.h"

dlhandle_t *
dylibLoad (const char *path, dlopt_t opt)
{
  void        *handle = NULL;
  const char  *pfx = "";
  char        tbuff [MAXPATHLEN];
  const char  *shlibext;
#if _lib_LoadLibraryW
  HMODULE     libhandle;
  char        npath [MAXPATHLEN];
  wchar_t     *wpath;
#endif

  if (path == NULL || ! *path) {
    return NULL;
  }

#if _lib_dlopen
  shlibext = sysvarsGetStr (SV_SHLIB_EXT);
  if ((opt & DYLIB_OPT_MAC_PREFIX) == DYLIB_OPT_MAC_PREFIX && isMacOS ()) {
    /* must have trailing slash */
    pfx = "/opt/local/lib/";
  }

  if (*path == '/') {
    stpecpy (tbuff, tbuff + sizeof (tbuff), path);
  } else {
    snprintf (tbuff, sizeof (tbuff), "%s%s%s", pfx, path, shlibext);
  }
fprintf (stderr, "dl-open: /%s/\n", tbuff);
  handle = dlopen (tbuff, RTLD_LAZY);
  if (handle == NULL) {
fprintf (stderr, "  dl-open: %s fail-a\n", tbuff);
    if ((opt & DYLIB_OPT_VERSION) == DYLIB_OPT_VERSION) {
      int   beg = DYLIB_ICU_BEG_VERS;
      int   end = DYLIB_ICU_END_VERS;

      if ((opt & DYLIB_OPT_AV) == DYLIB_OPT_AV) {
        beg = DYLIB_AV_BEG_VERS;
        end = DYLIB_AV_END_VERS;
      }

      /* ubuntu does not use plain .so links */
      for (int i = beg; i < end; ++i) {
        snprintf (tbuff, sizeof (tbuff), "%s%s%s.%d", pfx, path, shlibext, i);
fprintf (stderr, "  try: %s\n", tbuff);
        handle = dlopen (tbuff, RTLD_LAZY);
        if (handle != NULL) {
fprintf (stderr, "dl-open: found %s at %d\n", tbuff, i);
          break;
        }
      }
    }
  }

#endif
#if _lib_LoadLibraryW
  stpecpy (npath, npath + sizeof (npath), path);
  pathDisplayPath (npath, sizeof (npath));
  wpath = osToWideChar (npath);
  libhandle = LoadLibraryW (wpath);
  handle = libhandle;
#endif
  mdextalloc (handle);
  if (handle == NULL) {
    fprintf (stderr, "ERR: dylib open %s failed: %d %s\n", path, errno, strerror (errno));
  }

  return handle;
}

void
dylibClose (dlhandle_t *handle)
{
#if _lib_LoadLibraryW
  HMODULE   libhandle = handle;
#endif

  if (handle == NULL) {
    return;
  }

#if _lib_dlopen
  mdextfree (handle);
# if ! defined (BDJ4_USING_SANITIZER)
  /* using the address sanitizer comes up with spurious leaks if the dynamic */
  /* library is closed */
  dlclose (handle);
# endif
#endif
#if _lib_LoadLibraryW
  mdextfree (handle);
  FreeLibrary (libhandle);
#endif
}

void *
dylibLookup (dlhandle_t *handle, const char *funcname)
{
  void      *addr = NULL;
#if _lib_LoadLibraryW
  HMODULE   libhandle = handle;
#endif

  if (handle == NULL || funcname == NULL || ! *funcname) {
    return NULL;
  }

#if _lib_dlopen
  addr = dlsym (handle, funcname);
#endif
#if _lib_LoadLibraryW
  addr = GetProcAddress (libhandle, funcname);
#endif

/* debugging */
#if 0
  if (addr == NULL) {
    fprintf (stderr, "sym lookup %s failed: %d %s\n", funcname, errno, strerror (errno));
#if _lib_LoadLibraryW
    fprintf (stderr, "sym lookup %s getlasterror: %ld\n", funcname, GetLastError() );
#endif
  } else {
    fprintf (stderr, "sym lookup %s OK\n", funcname);
  }
#endif /* debug */

  return addr;
}

int
dylibCheckVersion (dlhandle_t *handle, const char *funcname, dlopt_t opt)
{
  void      *addr = NULL;
  int       version = -1;
  char      tbuff [MAXPATHLEN];

  if ((opt & DYLIB_OPT_VERSION) == DYLIB_OPT_VERSION) {
    int   beg = DYLIB_ICU_BEG_VERS;
    int   end = DYLIB_ICU_END_VERS;

    /* the ICU library versions the linkage... */
    for (int i = beg; i < end; ++i) {
      snprintf (tbuff, sizeof (tbuff), "%s_%d", funcname, i);
      addr = dylibLookup (handle, tbuff);
      if (addr != NULL) {
fprintf (stderr, "found %s at %d\n", funcname, i);
        version = i;
        break;
      }
    }
  }

  return version;
}
