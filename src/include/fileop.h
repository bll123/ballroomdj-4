#ifndef INC_FILEOP_H
#define INC_FILEOP_H

#include "config.h"
#include <stdio.h>
#include <time.h>

bool  fileopFileExists (const char *fname);
ssize_t fileopSize (const char *fname);
time_t  fileopModTime (const char *fname);
void  fileopSetModTime (const char *fname, time_t tm);
bool  fileopIsDirectory (const char *fname);
int   fileopDelete (const char *fname);
FILE  * fileopOpen (const char *fname, const char *mode);

#endif /* INC_FILEOP_H */
