/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_FILESHARED_H
#define INC_FILESHARED_H

typedef struct filehandle fileshared_t;

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  FILE_OPEN_APPEND,
  FILE_OPEN_TRUNCATE,
};

fileshared_t  *fileSharedOpen (const char *fname, int truncflag);
ssize_t       fileSharedWrite (fileshared_t *fileHandle, const char *data, size_t len);
void          fileSharedClose (fileshared_t *fileHandle);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_FILESHARED_H */
