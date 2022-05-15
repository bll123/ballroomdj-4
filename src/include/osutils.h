#ifndef INC_OSUTILS_H
#define INC_OSUTILS_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "config.h"

#include <sys/types.h>
#include <dirent.h>
#include <wchar.h>

#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif
#if _hdr_windows
# include <windows.h>
#endif

enum {
  OS_PROC_NONE    = 0x0000,
  OS_PROC_DETACH  = 0x0001,
  OS_PROC_WAIT    = 0x0002,
};

typedef struct {
  DIR       *dh;
  char      *dirname;
#if _typ_HANDLE
  HANDLE    dhandle;
#endif
} dirhandle_t;

#if _lib_MultiByteToWideChar
# define OS_FS_CHAR_TYPE wchar_t
#else
# define OS_FS_CHAR_TYPE char
#endif
#define OS_FS_CHAR_SIZE sizeof (OS_FS_CHAR_TYPE)

double        dRandom (void);
void          sRandom (void);
pid_t         osProcessStart (char *targv[], int flags,
                void **handle, char *outfname);
pid_t         osProcessPipe (const char *targv[], int flags, char *rbuff, size_t sz);
void          * osToFSFilename (const char *fname);
char          * osFromFSFilename (const void *fname);
dirhandle_t   * osDirOpen (const char *dir);
char          * osDirIterate (dirhandle_t *dirh);
void          osDirClose (dirhandle_t *dirh);
int           osSetEnv (const char *name, const char *value);
void          osSetStandardSignals (void (*sigHandler)(int));
void          osCatchSignal (void (*sigHandler)(int), int signal);
void          osIgnoreSignal (int signal);
void          osDefaultSignal (int signal);
char          *osGetLocale (char *buff, size_t sz);
char *        osRunProgram (const char *prog, ...);

/* system specific functions in separate files */
char          *osRegistryGet (char *key, char *name);
char          *osGetSystemFont (const char *gsettingspath);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INC_OSUTILS_H */
