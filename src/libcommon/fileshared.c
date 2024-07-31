/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if _hdr_io
# include <io.h>
#endif
#if _hdr_windows
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "bdjstring.h"
#include "fileshared.h"
#include "mdebug.h"
#include "osutils.h"
#include "sysvars.h"

enum {
  FLUSH_COUNT = 20,
};

typedef union filehandle {
#if _typ_HANDLE
  HANDLE  handle;
#endif
  FILE    *fh;
  int     count;
} fileshared_t;

fileshared_t *
fileSharedOpen (const char *fname, int truncflag)
{
  fileshared_t  *fhandle;

#if _lib_CreateFileW
  wchar_t   *wfname = NULL;
  HANDLE    handle;
  DWORD     cd;
#else
  FILE        *fh;
  int         flags;
#endif

  if (fname == NULL || ! *fname) {
    return NULL;
  }

  fhandle = mdmalloc (sizeof (fileshared_t));
  fhandle->fh = NULL;
  fhandle->count = 0;

#if _lib_CreateFileW
  cd = OPEN_ALWAYS;
  if (truncflag == FILE_OPEN_TRUNCATE) {
    cd = CREATE_ALWAYS;
  }

  wfname = osToWideChar (fname);
  handle = CreateFileW (wfname,
      FILE_APPEND_DATA,
      FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      cd,
      FILE_ATTRIBUTE_NORMAL,
      NULL);
  fhandle->handle = handle;
  if (handle == NULL) {
    dataFree (fhandle);
    fhandle = NULL;
  }
  dataFree (wfname);
  mdextfopen (handle);

#else

  /* not windows */

  flags = O_WRONLY | O_APPEND | O_CREAT;
# if _define_O_CLOEXEC
  flags |= O_CLOEXEC;
# endif
  if (truncflag == FILE_OPEN_TRUNCATE) {
    flags |= O_TRUNC;
  }

  fh = fopen (fname, "a");
  mdextfopen (fh);
  fhandle->fh = fh;
  if (fh == NULL) {
    dataFree (fhandle);
    fhandle = NULL;
  }
#endif

  return fhandle;
}

ssize_t
fileSharedWrite (fileshared_t *fhandle, const char *data, size_t len)
{
  ssize_t rc;
#if _lib_WriteFile
  DWORD   wlen;
#endif

  if (fhandle == NULL) {
    return -1;
  }

#if _lib_WriteFile
  rc = WriteFile (fhandle->handle, data, len, &wlen, NULL);
#else
  rc = fwrite (data, len, 1, fhandle->fh);
#endif

  /* on linux, flushing each time is reasonably fast */
  /* on MacOS, flushing each time is very slow */
  /* windows has not been tested */
  if (isLinux () || fhandle->count == FLUSH_COUNT) {
#if _lib_FlushFileBuffers
    FlushFileBuffers (fhandle->handle);
#else
    fflush (fhandle->fh);
#endif
    fhandle->count = 0;
  }
  ++fhandle->count;

  return rc;
}

void
fileSharedClose (fileshared_t *fhandle)
{
  if (fhandle == NULL) {
    return;
  }

#if _lib_CloseHandle
  mdextfclose (fhandle->handle);
  CloseHandle (fhandle->handle);
#else
  mdextfclose (fhandle->fh);
  fclose (fhandle->fh);
#endif
  dataFree (fhandle);
  return;
}

