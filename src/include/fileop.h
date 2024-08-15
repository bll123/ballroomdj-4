/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_FILEOP_H
#define INC_FILEOP_H

#include "config.h"
#include <stdio.h>
#include <time.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

bool    fileopFileExists (const char *fname);
ssize_t fileopSize (const char *fname);
time_t  fileopModTime (const char *fname);
void    fileopSetModTime (const char *fname, time_t tm);
time_t  fileopCreateTime (const char *fname);
bool    fileopIsDirectory (const char *fname);
int     fileopDelete (const char *fname);
FILE    * fileopOpen (const char *fname, const char *mode);
void    fileopSync (FILE *fh);
bool    fileopIsAbsolutePath (const char *fname);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_FILEOP_H */
