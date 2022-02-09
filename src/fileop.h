#ifndef INC_FILEOP_H
#define INC_FILEOP_H

bool  fileopExists (const char *fname);
bool  fileopIsDirectory (const char *fname);
int   fileopDelete (const char *fname);
int   fileopMakeDir (const char *dirname);

#endif /* INC_FILEOP_H */
