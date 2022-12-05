/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "osdir.h"
#include "osutils.h"

dirhandle_t *
osDirOpen (const char *dirname)
{
  dirhandle_t   *dirh;

  dirh = malloc (sizeof (dirhandle_t));
  assert (dirh != NULL);
  dirh->dirname = NULL;
  dirh->dh = NULL;

#if _lib_FindFirstFileW
  {
    size_t        len = 0;

    dirh->dhandle = INVALID_HANDLE_VALUE;
    len = strlen (dirname) + 3;
    dirh->dirname = malloc (len);
    strlcpy (dirh->dirname, dirname, len);
    stringTrimChar (dirh->dirname, '/');
    strlcat (dirh->dirname, "/*", len);
  }
#else
  dirh->dh = opendir (dirname);
#endif

  return dirh;
}

char *
osDirIterate (dirhandle_t *dirh)
{
  char      *fname;

#if _lib_FindFirstFileW
  WIN32_FIND_DATAW filedata;
  BOOL             rc;

  if (dirh->dhandle == INVALID_HANDLE_VALUE) {
    wchar_t         *wdirname;

    wdirname = osToWideChar (dirh->dirname);
    dirh->dhandle = FindFirstFileW (wdirname, &filedata);
    rc = 0;
    if (dirh->dhandle != INVALID_HANDLE_VALUE) {
      rc = 1;
    }
    free (wdirname);
  } else {
    rc = FindNextFileW (dirh->dhandle, &filedata);
  }

  fname = NULL;
  if (rc != 0) {
    fname = osFromWideChar (filedata.cFileName);
  }
#else
  struct dirent   *dirent;

  dirent = readdir (dirh->dh);
  fname = NULL;
  if (dirent != NULL) {
    fname = strdup (dirent->d_name);
  }
#endif

  return fname;
}


void
osDirClose (dirhandle_t *dirh)
{
#if _lib_FindFirstFileW
  if (dirh->dhandle != INVALID_HANDLE_VALUE) {
    FindClose (dirh->dhandle);
  }
#else
  closedir (dirh->dh);
#endif
  dataFree (dirh->dirname);
  dirh->dirname = NULL;
  free (dirh);
}

