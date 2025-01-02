/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>
#if _hdr_tchar
# include <tchar.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "osenv.h"
#include "osutils.h"

void
osGetEnv (const char *name, char *buff, size_t sz)
{
#if _lib__wgetenv_s
  wchar_t     *wname;
  wchar_t     wenv [MAXPATHLEN];
  char        *tenv;
  size_t      rv;

  *buff = '\0';
  wname = osToWideChar (name);
  _wgetenv_s (&rv, wenv, sizeof (wenv), wname);
  mdfree (wname);
  if (rv > 0) {
    tenv = osFromWideChar (wenv);
    stpecpy (buff, buff + sz, tenv);
    mdfree (tenv);
  }
#else
  char    *tptr;

  *buff = '\0';
  tptr = getenv (name);
  if (tptr != NULL) {
    stpecpy (buff, buff + sz, tptr);
  }
#endif
}

int
osSetEnv (const char *name, const char *value)
{
  int     rc;

#if _lib__wputenv_s
  {
    wchar_t *wname;
    wchar_t *wvalue;

    wname = osToWideChar (name);
    wvalue = osToWideChar (value);
    rc = _wputenv_s (wname, wvalue);
    mdfree (wname);
    mdfree (wvalue);
  }
#elif _lib_setenv
  /* setenv is better */
  if (value != NULL && *value) {
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

