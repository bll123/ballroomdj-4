/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct filehandle fileshared_t;

enum {
  FILESH_OPEN_APPEND,
  FILESH_OPEN_TRUNCATE,
  FILESH_OPEN_READ,
  FILESH_OPEN_READ_WRITE,
  FILESH_NO_SYNC,
  FILESH_SYNC,
  FILESH_NO_FLUSH,
  FILESH_FLUSH,
};

NODISCARD fileshared_t  *fileSharedOpen (const char *fname, int openmode, int flushflag);
ssize_t       fileSharedWrite (fileshared_t *fileHandle, const char *data, size_t len);
ssize_t       fileSharedRead (fileshared_t *fileHandle, char *data, size_t len);
char          *fileSharedGet (fileshared_t *fileHandle, char *data, size_t maxlen);
int           fileSharedSeek (fileshared_t *fileHandle, size_t offset, int mode);
ssize_t       fileSharedTell (fileshared_t *fileHandle);
void          fileSharedFlush (fileshared_t *fileHandle, int syncflag);
void          fileSharedClose (fileshared_t *fileHandle);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

