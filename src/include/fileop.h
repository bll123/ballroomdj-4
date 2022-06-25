#ifndef INC_FILEOP_H
#define INC_FILEOP_H

#include "config.h"
#include <stdio.h>
#include <time.h>

bool  fileopFileExists (const char *fname);
ssize_t fileopSize (const char *fname);
time_t  fileopModTime (const char *fname);
bool  fileopIsDirectory (const char *fname);
int   fileopDelete (const char *fname);
int   fileopMakeDir (const char *dirname);
FILE  * fileopOpen (const char *fname, const char *mode);

#endif /* INC_FILEOP_H */
