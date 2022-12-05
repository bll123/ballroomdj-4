/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_OSDIR_H
#define INC_OSDIR_H

#include "config.h"

#include <dirent.h>

#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif
#if _hdr_windows
# include <windows.h>
#endif

typedef struct {
  DIR       *dh;
  char      *dirname;
#if _typ_HANDLE
  HANDLE    dhandle;
#endif
} dirhandle_t;

dirhandle_t   * osDirOpen (const char *dir);
char          * osDirIterate (dirhandle_t *dirh);
void          osDirClose (dirhandle_t *dirh);

#endif /* INC_OSDIR_H */
