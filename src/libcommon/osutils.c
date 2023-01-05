/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
#include <stdarg.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "osutils.h"
#include "tmutil.h"

void
osGetEnv (const char *name, char *buff, size_t sz)
{
#if _lib__wgetenv
  wchar_t     *wname;
  wchar_t     *wenv;
  char        *tenv;

  *buff = '\0';
  wname = osToWideChar (name);
  wenv = _wgetenv (wname);
  mdfree (wname);
  tenv = osFromWideChar (wenv);
  strlcpy (buff, tenv, sz);
  mdfree (tenv);
#else
  char    *tptr;

  *buff = '\0';
  tptr = getenv (name);
  if (tptr != NULL) {
    strlcpy (buff, tptr, sz);
  }
#endif
}

int
osSetEnv (const char *name, const char *value)
{
  int     rc;

  /* setenv is better */
#if _lib_setenv
  if (*value) {
    rc = setenv (name, value, 1);
  } else {
    rc = unsetenv (name);
  }
#else
  {
    char    tbuff [4096];
    snprintf (tbuff, sizeof (tbuff), "%s=%s", name, value);
    rc = putenv (tbuff);
  }
#endif
  return rc;
}

#if _lib_symlink

int
osCreateLink (const char *target, const char *linkpath)
{
  int rc;

  rc = symlink (target, linkpath);
  return rc;
}

bool
osIsLink (const char *path)
{
  struct stat statbuf;
  int         rc;

  rc = lstat (path, &statbuf);
  if (rc == 0 && (statbuf.st_mode & S_IFMT) != S_IFLNK) {
    rc = -1;
  }
  return (rc == 0);
}

#endif
