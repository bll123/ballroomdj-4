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

#include <dirent.h>

#if __has_include (<windows.h>)
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "osdir.h"
#include "osutils.h"

typedef struct dirhandle {
  DIR       *dh;
  char      *dirname;
#if _typ_HANDLE
  HANDLE    dhandle;
#endif
} dirhandle_t;

[[nodiscard]]
dirhandle_t *
osDirOpen (const char *dirname)
{
  dirhandle_t   *dirh;

  dirh = mdmalloc (sizeof (dirhandle_t));
  dirh->dirname = NULL;
  dirh->dh = NULL;

#if _lib_FindFirstFileW
  {
    size_t        len = 0;
    char          *p;
    char          *end;

    dirh->dhandle = INVALID_HANDLE_VALUE;
    len = strlen (dirname) + 3;
    dirh->dirname = mdmalloc (len);
    p = dirh->dirname;
    end = dirh->dirname + len;
    p = stpecpy (p, end, dirname);
    stringTrimChar (dirh->dirname, '/');
    p = stpecpy (p, end, "/*");
  }
#else
  dirh->dirname = mdstrdup (dirname);
  dirh->dh = opendir (dirname);
  mdextfopen (dirh->dh);
#endif

  return dirh;
}

[[nodiscard]]
char *
osDirIterate (dirhandle_t *dirh)
{
  char      *fname = NULL;
#if _lib_FindFirstFileW
  WIN32_FIND_DATAW filedata;
  BOOL             rc;
#else
  struct dirent   *dirent = NULL;
#endif

  if (dirh == NULL) {
    return NULL;
  }

#if _lib_FindFirstFileW
  if (dirh->dhandle == INVALID_HANDLE_VALUE) {
    wchar_t         *wdirname;

    wdirname = osToWideChar (dirh->dirname);
    dirh->dhandle = FindFirstFileW (wdirname, &filedata);
    mdextfopen (dirh->dhandle);
    rc = 0;
    if (dirh->dhandle != INVALID_HANDLE_VALUE) {
      rc = 1;
    }
    mdfree (wdirname);
  } else {
    rc = FindNextFileW (dirh->dhandle, &filedata);
  }

  fname = NULL;
  if (rc != 0) {
    fname = osFromWideChar (filedata.cFileName);
  }
#else
  if (dirh->dh == NULL) {
    return NULL;
  }

  dirent = readdir (dirh->dh);
  fname = NULL;
  if (dirent != NULL) {
    fname = mdstrdup (dirent->d_name);
  }
#endif

  return fname;
}

void
osDirClose (dirhandle_t *dirh)
{
  if (dirh == NULL) {
    return;
  }

#if _lib_FindFirstFileW
  if (dirh->dhandle != INVALID_HANDLE_VALUE) {
    mdextfclose (dirh->dhandle);
    FindClose (dirh->dhandle);
  }
  dirh->dhandle = INVALID_HANDLE_VALUE;
#else
  if (dirh->dh != NULL) {
    mdextfclose (dirh->dh);
    closedir (dirh->dh);
  }
  dirh->dh = NULL;
#endif
  dataFree (dirh->dirname);
  dirh->dirname = NULL;
  mdfree (dirh);
}
