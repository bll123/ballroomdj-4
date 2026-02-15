/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <wchar.h>

#if __has_include (<windows.h>)
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "osutils.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "sysvars.h"

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
pathRealPath (char *path, size_t sz)
{
#if _lib_realpath
  char      tbuff [BDJ4_PATH_MAX];

  (void) ! realpath (path, tbuff);
  stpecpy (path, path + sz, tbuff);
#endif
#if ! lib_realpath && _lib_GetFullPathNameW
  wchar_t   *wfrom;
  wchar_t   wto [BDJ4_PATH_MAX];
  char      *tto;

  pathDisplayPath (path, sz);
  wfrom = osToWideChar (path);
  (void) ! GetFullPathNameW (wfrom, BDJ4_PATH_MAX, wto, NULL);
  mdfree (wfrom);
  /* convert the path to use the short name so that account */
  /* names with international characters will work */
  wfrom = wcsdup (wto);
  (void) ! GetShortPathNameW (wfrom, wto, BDJ4_PATH_MAX);
  tto = osFromWideChar (wto);
  stpecpy (path, path + sz, tto);
  mdfree (wfrom);
  mdfree (tto);
#endif
}

void
pathLongPath (char *path, size_t sz)
{
#if _lib_GetLongPathNameW
  wchar_t   *wfrom;
  wchar_t   wto [BDJ4_PATH_MAX];
  char      *tto;

  wfrom = osToWideChar (path);
  (void) ! GetFullPathNameW (wfrom, BDJ4_PATH_MAX, wto, NULL);
  mdfree (wfrom);
  /* convert the path to use the long name */
  wfrom = wcsdup (wto);
  (void) ! GetLongPathNameW (wfrom, wto, BDJ4_PATH_MAX);
  tto = osFromWideChar (wto);
  stpecpy (path, path + sz, tto);
  mdfree (wfrom);
  mdfree (tto);
#endif

  return;
}
