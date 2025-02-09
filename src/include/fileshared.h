/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_FILESHARED_H
#define INC_FILESHARED_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct filehandle fileshared_t;

enum {
  FILE_OPEN_APPEND,
  FILE_OPEN_TRUNCATE,
  FILE_OPEN_READ,
  FILE_OPEN_READ_WRITE,
};

fileshared_t  *fileSharedOpen (const char *fname, int openmode);
ssize_t       fileSharedWrite (fileshared_t *fileHandle, const char *data, size_t len);
ssize_t       fileSharedRead (fileshared_t *fileHandle, char *data, size_t len);
char          *fileSharedGet (fileshared_t *fileHandle, char *data, size_t maxlen);
int           fileSharedSeek (fileshared_t *fileHandle, size_t offset, int mode);
ssize_t       fileSharedTell (fileshared_t *fileHandle);
void          fileSharedFlush (fileshared_t *fileHandle);
void          fileSharedClose (fileshared_t *fileHandle);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_FILESHARED_H */
