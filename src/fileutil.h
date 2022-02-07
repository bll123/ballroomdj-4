#ifndef INC_FILEUTIL_H
#define INC_FILEUTIL_H

#include "config.h"

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

typedef union {
#if _typ_HANDLE
  HANDLE  handle;
#endif
  int     fd;
} filehandle_t;

int           fileOpenShared (const char *fname, int truncflag,
                  filehandle_t *fileHandle);
ssize_t       fileWriteShared (filehandle_t *fileHandle, char *data, size_t len);
void          fileCloseShared (filehandle_t *fileHandle);

#endif /* INC_FILEUTIL_H */
