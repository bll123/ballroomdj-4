/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */

/*
 * msys2 does not properly convert the command line arguments into utf-8.
 * Therefore the windows routines are used to grab the wide arguments
 * and the conversion is done ourselves.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#if __has_include (<windows.h>)
# define NOCRYPT 1
# define NOGDI 1
# include <windows.h>
#endif

#include "bdj4arg.h"
#include "mdebug.h"
#include "osutils.h"

typedef struct bdj4arg {
  char      **utf8argv;
  int       nargc;
} bdj4arg_t;

NODISCARD
bdj4arg_t *
bdj4argInit (int argc, char *argv [])
{
  bdj4arg_t   *bdj4arg;
#if _lib_GetCommandLineW
  wchar_t   **wargv;
  int       targc;
#endif

  bdj4arg = mdmalloc (sizeof (bdj4arg_t));
  bdj4arg->nargc = argc;
  bdj4arg->utf8argv = NULL;
  if (argc > 0) {
    bdj4arg->utf8argv = mdmalloc (sizeof (char *) * argc);
  }
#if _lib_GetCommandLineW
  wargv = CommandLineToArgvW (GetCommandLineW(), &targc);
  for (int i = 0; i < argc; ++i) {
    bdj4arg->utf8argv [i] = osFromWideChar (wargv [i]);
  }
  LocalFree (wargv);
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

  if (bdj4arg->utf8argv != NULL) {
    for (int i = 0; i < bdj4arg->nargc; ++i) {
      dataFree (bdj4arg->utf8argv [i]);
    }
  }
  dataFree (bdj4arg->utf8argv);
  dataFree (bdj4arg);
}

/* getopt_long_only re-arranges the argv array, so we need to have */
/* our own copy of it. */
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
