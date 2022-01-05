#ifndef INC_FILEOP_H
#define INC_FILEOP_H

#if _hdr_windows
# include <windows.h>
#endif

int           fileopExists (char *);
int           fileopDelete (const char *fname);
int           fileopCopy (char *, char *);
int           fileopCopyLink (char *, char *);
int           fileopMove (char *, char *);
int           fileopMakeDir (char *dirname);

#endif /* INC_FILEOP_H */
