/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if __has_include (<io.h>)
# include <io.h>
#endif
#if __has_include (<windows.h>)
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "bdjstring.h"
#include "fileop.h"
#include "fileshared.h"
#include "mdebug.h"
#include "nodiscard.h"
#include "osutils.h"
#include "sysvars.h"

enum {
  FLUSH_COUNT = 20,
};

typedef struct filehandle {
#if _typ_HANDLE
  HANDLE  handle;
#endif
  FILE          *fh;
  _Atomic(int)  count;
  int           openmode;
  int           flushflag;
} fileshared_t;

NODISCARD
fileshared_t *
fileSharedOpen (const char *fname, int openmode, int flushflag)
{
  fileshared_t  *fhandle = NULL;

#if _lib_CreateFileW
  wchar_t     *wfname = NULL;
  HANDLE      handle = NULL;
  DWORD       access;
  DWORD       cd;
#else
  FILE        *fh;
  const char  *mode;
#endif

  if (fname == NULL || ! *fname) {
    return NULL;
  }

  fhandle = mdmalloc (sizeof (fileshared_t));
  fhandle->count = 0;
  fhandle->fh = NULL;
  fhandle->openmode = openmode;
  fhandle->flushflag = flushflag;
#if _typ_HANDLE
  fhandle->handle = NULL;
#endif

#if _lib_CreateFileW
  access = FILE_APPEND_DATA;
  cd = OPEN_ALWAYS;
  if (openmode == FILESH_OPEN_TRUNCATE) {
    cd = CREATE_ALWAYS;
  }
  if (openmode == FILESH_OPEN_READ) {
    access = FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA;
    cd = OPEN_ALWAYS;
  }
  if (openmode == FILESH_OPEN_READ_WRITE) {
    access = FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA;
    access |= FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA;
    cd = OPEN_ALWAYS;
  }

  wfname = osToWideChar (fname);
  handle = CreateFileW (wfname,
      access,
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

  mode = "ab";
  if (openmode == FILESH_OPEN_TRUNCATE) {
    mode = "wb";
  }
  if (openmode == FILESH_OPEN_READ) {
    mode = "rb";
  }
  if (openmode == FILESH_OPEN_READ_WRITE) {
    mode = "rb+";
    if (! fileopFileExists (fname)) {
      mode = "wb+";
    }
  }

  fh = fopen (fname, mode);
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

  if (fhandle == NULL) {
    return -1;
  }

#if _lib_WriteFile
  {
    DWORD   wlen;

    if (fhandle->handle == NULL) {
      return -1;
    }
    rc = WriteFile (fhandle->handle, data, len, &wlen, NULL);
    rc = 0;
    if (len == wlen) {
      rc = 1;
    }
  }
#else
  if (fhandle->fh == NULL) {
    return -1;
  }
  rc = fwrite (data, len, 1, fhandle->fh);
#endif

  /* on linux, flushing each time is reasonably fast */
  if (isLinux ()) {
    fhandle->count = FLUSH_COUNT;
  }
  /* on MacOS, flushing each time is very slow */
  /* windows flush-file-buffers does a sync */
  if (fhandle->flushflag == FILESH_FLUSH && fhandle->count >= FLUSH_COUNT) {
    fileSharedFlush (fhandle, FILESH_NO_SYNC);
    fhandle->count = 0;
  }
  ++fhandle->count;

  return rc;
}

ssize_t
fileSharedRead (fileshared_t *fhandle, char *data, size_t len)
{
  ssize_t rc;

  if (fhandle == NULL) {
    return -1;
  }
  fhandle->count = 0;

#if _lib_ReadFile
  {
    DWORD   rlen;

    if (fhandle->handle == NULL) {
      return -1;
    }
    rc = ReadFile (fhandle->handle, data, len, &rlen, NULL);
    rc = 0;
    if (rlen == len) {
      rc = 1;
    }
  }
#else
  if (fhandle->fh == NULL) {
    return -1;
  }
  rc = fread (data, len, 1, fhandle->fh);
#endif

  return rc;
}

char *
fileSharedGet (fileshared_t *fhandle, char *data, size_t maxlen)
{
  char    *p;

  if (fhandle == NULL) {
    return NULL;
  }

  *data = '\0';

#if _lib_ReadFile
  {
    size_t  loc;
    DWORD   rlen;
    int     rc;

    if (fhandle->handle == NULL) {
      return NULL;
    }
    /* get the current location */
    loc = SetFilePointer (fhandle->handle, 0, NULL, FILE_CURRENT);
    p = NULL;
    rc = ReadFile (fhandle->handle, data, maxlen, &rlen, NULL);
    if (rc == 1) {
      p = strchr (data, '\n');
      if (p != NULL) {
        ++p;
        if (p < data + maxlen) {
          *p = '\0';
          loc += p - data;
          /* rewind back to the char after the cr/newline */
          fileSharedSeek (fhandle, loc, SEEK_SET);
          p = data;
        } else {
          p = NULL;
        }
      }

      if (p == NULL) {
        *data = '\0';
        fileSharedSeek (fhandle, - rlen, SEEK_SET);
        p = NULL;
      }
    }
  }
#else
  if (fhandle->fh == NULL) {
    return NULL;
  }
  p = fgets (data, maxlen, fhandle->fh);
#endif

  return data;
}

int
fileSharedSeek (fileshared_t *fhandle, size_t offset, int mode)
{
  ssize_t rc;

  if (fhandle == NULL) {
    return -1;
  }

#if _lib_SetFilePointer
  switch (mode) {
    case SEEK_SET: {
      mode = FILE_BEGIN;
      break;
    }
    case SEEK_CUR: {
      mode = FILE_CURRENT;
      break;
    }
    case SEEK_END: {
      mode = FILE_END;
      break;
    }
  }
  rc = SetFilePointer (fhandle->handle, offset, NULL, mode);
  if (rc == INVALID_SET_FILE_POINTER) {
    rc = -1;
  } else {
    rc = 0;
  }
#else
  rc = fileopSeek (fhandle->fh, offset, mode);
#endif

  return rc;
}

/* only used in the check suite */
ssize_t
fileSharedTell (fileshared_t *fhandle)  /* TESTING */
{
  ssize_t rc;

  if (fhandle == NULL) {
    return -1;
  }

#if _lib_SetFilePointer
  if (fhandle->handle == NULL) {
    return -1;
  }
  rc = SetFilePointer (fhandle->handle, 0, NULL, FILE_CURRENT);
#else
  if (fhandle->fh == NULL) {
    return -1;
  }
  rc = fileopTell (fhandle->fh);
#endif

  return rc;
}


void
fileSharedFlush (fileshared_t *fhandle, int syncflag)
{
  if (fhandle == NULL) {
    return;
  }

#if _lib_FlushFileBuffers
  if (fhandle->handle == NULL) {
    return;
  }
  /* windows flush-file-buffers does a sync */
  FlushFileBuffers (fhandle->handle);
#else
  if (fhandle->fh == NULL) {
    return;
  }
  fflush (fhandle->fh);
  if (syncflag == FILESH_SYNC && fhandle->openmode != FILESH_OPEN_READ) {
    fsync (fileno (fhandle->fh));
  }
#endif
  fhandle->count = 0;
}

void
fileSharedClose (fileshared_t *fhandle)
{
  if (fhandle == NULL) {
    return;
  }

  if (fhandle->openmode != FILESH_OPEN_READ) {
    /* windows does not necessarily flush the buffers to disk on close */
    fileSharedFlush (fhandle, FILESH_SYNC);
  }
#if _lib_CloseHandle
  if (fhandle->handle == NULL) {
    return;
  }
  mdextfclose (fhandle->handle);
  CloseHandle (fhandle->handle);
#else
  if (fhandle->fh == NULL) {
    return;
  }
  mdextfclose (fhandle->fh);
  fclose (fhandle->fh);
#endif
  dataFree (fhandle);
  return;
}

