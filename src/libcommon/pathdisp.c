/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wchar.h>

#if __has_include (<windows.h>)
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "pathdisp.h"

/* use #if _WIN32 here so there is no dependency on sysvars */

void
pathDisplayPath (char *path, size_t sz)
{
#if _WIN32
  for (size_t i = 0; i < sz; ++i) {
    if (path [i] == '\0') {
      break;
    }
    if (path [i] == '/') {
      path [i] = '\\';
    }
  }
#endif
  return;
}

void
pathNormalizePath (char *buff, size_t len)
{
#if _WIN32
  for (size_t i = 0; i < len; ++i) {
    if (buff [i] == '\0') {
      break;
    }
    if (buff [i] == '\\') {
      buff [i] = '/';
    }
  }
#endif
  return;
}

