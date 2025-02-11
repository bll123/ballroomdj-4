/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_FILEMANIP_H
#define INC_FILEMANIP_H

#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int     filemanipCopy (const char *from, const char *to);
int     filemanipLinkCopy (const char *from, const char *to);
int     filemanipMove (const char *from, const char *to);
void    filemanipBackup (const char *fname, int count);
void    filemanipRenameAll (const char *ofname, const char *nfname);
void    filemanipDeleteAll (const char *name);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_FILEMANIP_H */
