#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if _hdr_io
# include <io.h>
#endif
#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "fileutil.h"
#include "bdjstring.h"

int
fileOpenShared (const char *fname, int truncflag, filehandle_t *fhandle)
{
  int         rc;

#if _lib_CreateFile
  HANDLE    handle;
  DWORD     cd;

  cd = OPEN_ALWAYS;
  if (truncflag) {
    cd = CREATE_ALWAYS;
  }

  rc = 0;
  handle = CreateFile (fname,
      FILE_APPEND_DATA,
      FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      cd,
      FILE_ATTRIBUTE_NORMAL,
      NULL);
  if (handle == NULL) {
    rc = -1;
  }
  fhandle->handle = handle;
#else
  int         fd;
  int         flags;

  flags = O_WRONLY | O_APPEND | O_CREAT;
# if _define_O_CLOEXEC
  flags |= O_CLOEXEC;
# endif
  if (truncflag) {
    flags |= O_TRUNC;
  }
  fd = open (fname, flags, 0600);
  fhandle->fd = fd;
  rc = fd;
#endif
  return rc;
}

ssize_t
fileWriteShared (filehandle_t *fhandle, char *data, size_t len)
{
  ssize_t rc;

#if _lib_WriteFile
  DWORD   wlen;
  rc = WriteFile(fhandle->handle, data, len, &wlen, NULL);
#else
  rc = write (fhandle->fd, data, len);
#endif
  return rc;
}

void
fileCloseShared (filehandle_t *fhandle)
{
#if _lib_CloseHandle
  CloseHandle (fhandle->handle);
#else
  close (fhandle->fd);
#endif
  return;
}

