#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "bdj4arg.h"
#include "mdebug.h"
#include "osutils.h"

#if _lib_GetCommandLineW
static wchar_t      **wargv = NULL;
static int          wargc = 0;
#endif

static bool         initialized = false;

void
bdj4argInit (void)
{
  if (! initialized) {
#if _lib_GetCommandLineW
    wargv = CommandLineToArgvW (GetCommandLineW(), &wargc);
#endif
    initialized = true;
  }
}

void
bdj4argCleanup (void)
{
  if (initialized) {
#if _lib_GetCommandLineW
    LocalFree (wargv);
#endif
    initialized = false;
  }
}

char *
bdj4argGet (int idx, const char *arg)
{
  char    *targ = NULL;

  if (! initialized) {
    return NULL;
  }

#if _lib_GetCommandLineW
  if (idx < 0 || idx >= wargc) {
    return NULL;
  }

  targ = osFromWideChar (wargv [idx]);
#else
  targ = (char *) arg;
#endif
  return targ;
}

void
bdj4argClear (void *targ)
{
#if _lib_GetCommandLineW
  bdj4initFreeArg (targ);
#endif
  return;
}

