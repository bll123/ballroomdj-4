/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wchar.h>

#include "bdjstring.h"
#include "mdebug.h"
#include "osdirutil.h"
#include "osutils.h"

int
osChangeDir (const char *path)
{
  int   rc;

#if _lib__wchdir
  wchar_t   *wpath;

  wpath = osToWideChar (path);
  rc = _wchdir (wpath);
  mdfree (wpath);
#else
  rc = chdir (path);
#endif
  return rc;
}

void
osGetCurrentDir (char *buff, size_t len)
{
#if _lib__wgetcwd
  wchar_t *wcwd;
  char    *tmp;

  wcwd = _wgetcwd (NULL, 0);
  mdextalloc (wcwd);
  tmp = osFromWideChar (wcwd);
  strlcpy (buff, tmp, len);
  mdextfree (wcwd);
  mdfree (tmp);
#else
  (void) ! getcwd (buff, len);
#endif
}

