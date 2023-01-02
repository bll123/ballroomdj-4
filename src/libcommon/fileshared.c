/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if _hdr_io
# include <io.h>
#endif
#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdjstring.h"
#include "fileshared.h"

typedef union filehandle {
#if _typ_HANDLE
  HANDLE  handle;
#endif
  int     fd;
} fileshared_t;

fileshared_t *
fileSharedOpen (const char *fname, int truncflag)
{
  fileshared_t  *fhandle;
#if _lib_CreateFile
  HANDLE    handle;
  DWORD     cd;
#else
  int         fd;
  int         flags;
#endif

  fhandle = malloc (sizeof (fileshared_t));

#if _lib_CreateFile
  cd = OPEN_ALWAYS;
  if (truncflag == FILE_OPEN_TRUNCATE) {
    cd = CREATE_ALWAYS;
  }

  handle = CreateFile (fname,
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
#else
  flags = O_WRONLY | O_APPEND | O_CREAT;
# if _define_O_CLOEXEC
  flags |= O_CLOEXEC;
# endif
  if (truncflag == FILE_OPEN_TRUNCATE) {
    flags |= O_TRUNC;
  }
  fd = open (fname, flags, 0600);
  fhandle->fd = fd;
  if (fd < 0) {
    dataFree (fhandle);
    fhandle = NULL;
  }
#endif
  return fhandle;
}

ssize_t
fileSharedWrite (fileshared_t *fhandle, char *data, size_t len)
{
  ssize_t rc;

  if (fhandle == NULL) {
    return -1;
  }

#if _lib_WriteFile
  DWORD   wlen;
  rc = WriteFile(fhandle->handle, data, len, &wlen, NULL);
#else
  rc = write (fhandle->fd, data, len);
#endif
  return rc;
}

void
fileSharedClose (fileshared_t *fhandle)
{
  if (fhandle == NULL) {
    return;
  }
#if _lib_CloseHandle
  CloseHandle (fhandle->handle);
#else
  close (fhandle->fd);
#endif
  dataFree (fhandle);
  return;
}

