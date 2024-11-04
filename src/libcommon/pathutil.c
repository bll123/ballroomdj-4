/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#if _hdr_windows
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "osutils.h"
#include "pathutil.h"

void
pathNormalizePath (char *buff, size_t len)
{
  for (size_t i = 0; i < len; ++i) {
    if (buff [i] == '\0') {
      break;
    }
    if (buff [i] == '\\') {
      buff [i] = '/';
    }
  }
}

/* remove all /./ ../dir components */
void
pathStripPath (char *buff, size_t len)
{
  size_t  j = 0;
  size_t  priorslash = 0;
  size_t  lastslash = 0;
  char    *p;

  pathNormalizePath (buff, len);
  j = 0;
  p = buff;
  for (size_t i = 0; i < len; ++i) {
    if (*p == '\0') {
      buff [j] = '\0';
      break;
    }
    if (*p == '/') {
      priorslash = lastslash;
      lastslash = j;
    }
    if ((j == 0 || j - 1 == lastslash) &&
        (len - i) > 1 && strncmp (p, "./", 2) == 0) {
      p += 2;
    }
    if ((j == 0 || j - 1 == lastslash) &&
        (len - i) > 2 && strncmp (p, "../", 3) == 0) {
      p += 3;
      j = priorslash + 1;
    }

    buff [j] = *p;
    ++j;
    ++p;
  }
}

void
pathRealPath (char *to, const char *from, size_t sz)
{
#if _lib_realpath
  (void) ! realpath (from, to);
#endif
#if _lib_GetFullPathNameW
  wchar_t   *wfrom;
  wchar_t   wto [MAXPATHLEN];
  char      *tto;

  wfrom = osToWideChar (from);
  (void) ! GetFullPathNameW (wfrom, MAXPATHLEN, wto, NULL);
  tto = osFromWideChar (wto);
  stpecpy (to, to + sz, tto);
  mdfree (wfrom);

#endif
}

