/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_FILESHARED_H
#define INC_FILESHARED_H

typedef union filehandle fileshared_t;

enum {
  FILE_OPEN_APPEND,
  FILE_OPEN_TRUNCATE,
};

fileshared_t  *fileSharedOpen (const char *fname, int truncflag);
ssize_t       fileSharedWrite (fileshared_t *fileHandle, char *data, size_t len);
void          fileSharedClose (fileshared_t *fileHandle);

#endif /* INC_FILESHARED_H */
