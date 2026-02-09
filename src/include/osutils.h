/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "config.h"
#include "nodiscard.h"

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

BDJ_NODISCARD wchar_t * osToWideChar (const char *buff);
BDJ_NODISCARD char    * osFromWideChar (const wchar_t *buff);
int     osCreateLink (const char *target, const char *linkpath);
bool    osIsLink (const char *path);
void    osSuspendSleep (void);
void    osResumeSleep (void);

/* system specific functions in separate files */
void  osRegistryGet (char *key, char *name, char *buff, size_t sz);
void  osGetSystemFont (const char *gsettingspath, char *buff, size_t sz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

