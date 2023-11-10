/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_OSUTILS_H
#define INC_OSUTILS_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "config.h"

#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif
#if _hdr_windows
# include <windows.h>
#endif

#if _lib_MultiByteToWideChar
# define OS_FS_CHAR_TYPE wchar_t
#else
# define OS_FS_CHAR_TYPE char
#endif
#define OS_FS_CHAR_SIZE sizeof (OS_FS_CHAR_TYPE)

wchar_t * osToWideChar (const char *buff);
char    * osFromWideChar (const wchar_t *buff);
int     osCreateLink (const char *target, const char *linkpath);
bool    osIsLink (const char *path);
void    osSuspendSleep (void);
void    osResumeSleep (void);

/* system specific functions in separate files */
char    *osRegistryGet (char *key, char *name);
char    *osGetSystemFont (const char *gsettingspath);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INC_OSUTILS_H */
