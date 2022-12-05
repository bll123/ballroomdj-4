/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_FILESHARED_H
#define INC_FILESHARED_H

#include "config.h"

#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
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

enum {
  FILE_OPEN_APPEND,
  FILE_OPEN_TRUNCATE,
};

int           fileOpenShared (const char *fname, int truncflag,
                  filehandle_t *fileHandle);
ssize_t       fileWriteShared (filehandle_t *fileHandle, char *data, size_t len);
void          fileCloseShared (filehandle_t *fileHandle);

#endif /* INC_FILESHARED_H */
