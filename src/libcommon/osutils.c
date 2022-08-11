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
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "osutils.h"
#include "tmutil.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"

int
osSetEnv (const char *name, const char *value)
{
  int     rc;
  char    tbuff [4096];

  /* setenv is better */
#if _lib_setenv
  rc = setenv (name, value, 1);
#else
  snprintf (tbuff, sizeof (tbuff), "%s=%s", name, value);
  rc = putenv (tbuff);
#endif
  return rc;
}

#pragma gcc diagnotic pop
#pragma clang diagnotic pop

#if _lib_symlink

int
osCreateLink (const char *target, const char *linkpath)
{
  int rc;

  rc = symlink (target, linkpath);
  return rc;
}

#endif
