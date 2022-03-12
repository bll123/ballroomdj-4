#ifndef INC_OSUTILS_H
#define INC_OSUTILS_H

#include "config.h"

#include <sys/types.h>
#include <dirent.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

enum {
  OS_PROC_NONE    = 0x0000,
  OS_PROC_DETACH  = 0x0001,
};

typedef struct {
  DIR       *dh;
  char      *dirname;
#if _typ_HANDLE
  HANDLE    dhandle;
#endif
} dirhandle_t;

double        dRandom (void);
void          sRandom (void);
pid_t         osProcessStart (char *targv[], int flags, void **handle);
#if _lib_MultiByteToWideChar
 wchar_t * osToWideString (const char *fname);
 char * osToUTF8String (const wchar_t *fname);
#endif
dirhandle_t   * osDirOpen (const char *dir);
char *        osDirIterate (dirhandle_t *dirh);
void          osDirClose (dirhandle_t *dirh);

#endif /* INC_OSUTILS_H */
