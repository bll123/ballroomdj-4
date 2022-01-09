#ifndef INC_FILEOP_H
#define INC_FILEOP_H

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

bool          fileopExists (char *);
int           fileopDelete (const char *fname);
int           fileopCopy (char *, char *);
int           fileopLinkCopy (char *, char *);
int           fileopMove (char *, char *);
int           fileopMakeDir (char *dirname);

#endif /* INC_FILEOP_H */
