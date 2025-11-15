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
#include "fileop.h"
#include "dylib.h"
#include "log.h"
#include "mdebug.h"
#include "osutils.h"
#include "pathdisp.h"
#include "sysvars.h"

static int dylibvers = 0;
static dlhandle_t * dylibBasicOpen (char *path, size_t sz);

dlhandle_t *
dylibLoad (const char *path, dlopt_t opt)
{
  void        *handle = NULL;
  char        npath [MAXPATHLEN];
  const char  *shlibext;
  const char  *pfx = "";

  if (path == NULL || ! *path) {
    return NULL;
  }

  shlibext = sysvarsGetStr (SV_SHLIB_EXT);

  if ((opt & DYLIB_OPT_MAC_PREFIX) == DYLIB_OPT_MAC_PREFIX && isMacOS ()) {
    const char  *tdir;
    const char  *tfn;
    bool        found = false;

    /* must have trailing slash */
    tdir = "/opt/local/lib/";
    if (fileopIsDirectory (tdir)) {
      tfn = "data/macos.homebrew";
      if (! fileopFileExists (tfn)) {
        pfx = tdir;
        found = true;
      }
    }
    tdir = "/opt/homebrew/lib/";
    if (! found && fileopIsDirectory (tdir)) {
      pfx = tdir;
    }
    tdir = "/usr/local/Homebrew/";
    if (! found && fileopIsDirectory (tdir)) {
      tdir = "/usr/local/lib/";
      pfx = tdir;
    }
  }

  if (fileopIsAbsolutePath (path)) {
    stpecpy (npath, npath + sizeof (npath), path);
  } else {
    snprintf (npath, sizeof (npath), "%s%s%s", pfx, path, shlibext);
  }

  handle = dylibBasicOpen (npath, sizeof (npath));

  if (handle == NULL) {
    if ((opt & DYLIB_OPT_VERSION) == DYLIB_OPT_VERSION) {
      int   beg = DYLIB_ICU_BEG_VERS;
      int   end = DYLIB_ICU_END_VERS;

      if ((opt & DYLIB_OPT_AV) == DYLIB_OPT_AV) {
        beg = DYLIB_AV_BEG_VERS;
        end = DYLIB_AV_END_VERS;
      }

      /* ubuntu(?) does not use plain .so links */
      /* there was some version of linux that did not */
      for (int i = beg; i < end; ++i) {
        if (isWindows ()) {
          snprintf (npath, sizeof (npath), "%s%s%d%s", pfx, path, i, shlibext);
        } else {
          snprintf (npath, sizeof (npath), "%s%s%s.%d", pfx, path, shlibext, i);
        }
        handle = dylibBasicOpen (npath, sizeof (npath));
        if (handle != NULL) {
          dylibvers = i;
          break;
        }
      }
    }
  }

  if (handle == NULL) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "WARN: dylib open %s failed: %d %s\n",
        path, errno, strerror (errno));
  } else {
    mdextalloc (handle);
  }

  return handle;
}

/* must be called immediately after dylibLoad() */
int
dylibVersion (void)         /* TESTING */
{
  return dylibvers;
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
        version = i;
        break;
      }
    }
  }

  return version;
}

/* internal routines */

static dlhandle_t *
dylibBasicOpen (char *path, size_t sz)
{
  dlhandle_t    *handle;
#if _lib_LoadLibraryW
  HMODULE     libhandle;
  wchar_t     *wpath;
#endif

#if _lib_dlopen
  handle = dlopen (path, RTLD_LAZY);
#endif
#if _lib_LoadLibraryW
  pathDisplayPath (path, sz);
  wpath = osToWideChar (path);
  libhandle = LoadLibraryW (wpath);
  handle = libhandle;
#endif

  return handle;
}
