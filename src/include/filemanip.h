/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_FILEMANIP_H
#define INC_FILEMANIP_H

#include "slist.h"

enum {
  FILEMANIP_FILES = 0x01,
  FILEMANIP_DIRS = 0x02,
};

int     filemanipCopy (const char *from, const char *to);
int     filemanipLinkCopy (const char *from, const char *to);
int     filemanipMove (const char *from, const char *to);
slist_t * filemanipBasicDirList (const char *dir, const char *extension);
slist_t * filemanipRecursiveDirList (const char *dir, int flags);
void    filemanipBackup (const char *fname, int count);
void    filemanipRenameAll (const char *ofname, const char *nfname);

#endif /* INC_FILEMANIP_H */
