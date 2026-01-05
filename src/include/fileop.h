/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "config.h"
#include <stdio.h>
#include <stdint.h>
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
bool    fileopIsAbsolutePath (const char *fname);
int64_t fileopTell (FILE *fh);
int fileopSeek (FILE *fh, int64_t offset, int whence);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

