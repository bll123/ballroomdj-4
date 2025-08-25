/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "config.h"

#include <stdbool.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
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
} /* extern C */
#endif

