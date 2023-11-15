#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4arg.h"
#include "mdebug.h"
#include "osutils.h"

typedef struct bdj4arg {
#if _lib_GetCommandLineW
  wchar_t   **wargv;
#endif
  char      **utf8argv;
  int       nargc;
} bdj4arg_t;

bdj4arg_t *
bdj4argInit (int argc, char *argv [])
{
  bdj4arg_t   *bdj4arg;

  bdj4arg = mdmalloc (sizeof (bdj4arg_t));
  bdj4arg->nargc = argc;
  bdj4arg->utf8argv = NULL;
  if (argc > 0) {
    bdj4arg->utf8argv = mdmalloc (sizeof (char *) * argc);
  }
#if _lib_GetCommandLineW
  bdj4arg->wargv = CommandLineToArgvW (GetCommandLineW(), &bdj4arg->nargc);
  for (int i = 0; i < argc; ++i) {
    bdj4arg->utf8argv [i] = osFromWideChar (bdj4arg->wargv [i]);
  }
#else
  for (int i = 0; i < argc; ++i) {
    bdj4arg->utf8argv [i] = mdstrdup (argv [i]);
  }
#endif

  return bdj4arg;
}

void
bdj4argCleanup (bdj4arg_t *bdj4arg)
{
  if (bdj4arg == NULL) {
    return;
  }

#if _lib_GetCommandLineW
  LocalFree (bdj4arg->wargv);
#endif
  if (bdj4arg->utf8argv != NULL) {
    for (int i = 0; i < bdj4arg->nargc; ++i) {
      dataFree (bdj4arg->utf8argv [i]);
    }
  }
  dataFree (bdj4arg->utf8argv);
  dataFree (bdj4arg);
}

char **
bdj4argGetArgv (bdj4arg_t *bdj4arg)
{
  if (bdj4arg == NULL) {
    return NULL;
  }

  return bdj4arg->utf8argv;
}

const char *
bdj4argGet (bdj4arg_t *bdj4arg, int idx, const char *arg)
{
  const char  *targ = NULL;

  if (bdj4arg == NULL) {
    return NULL;
  }

  if (idx < 0 || idx >= bdj4arg->nargc) {
    return NULL;
  }

  targ = bdj4arg->utf8argv [idx];
  return targ;
}
