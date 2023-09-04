/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include <winsock2.h>
#include <windows.h>

#include "bdj4.h"
#include "mdebug.h"
#include "osutils.h"

char *
osRegistryGet (char *key, char *name)
{
  char    *rval = NULL;
  HKEY    hkey;
  LSTATUS rc;
  unsigned char buff [512];
  DWORD   len = 512;

  *buff = '\0';

  rc = RegOpenKeyEx (
      HKEY_CURRENT_USER,
      key,
      0,
      KEY_QUERY_VALUE,
      &hkey
      );

  if (rc == ERROR_SUCCESS) {
    rc = RegQueryValueEx (
        hkey,
        name,
        NULL,
        NULL,
        buff,
        &len
        );
    if (rc == ERROR_SUCCESS) {
      rval = mdstrdup ((char *) buff);
    }
    RegCloseKey (hkey);
  }

  return rval;
}

char *
osGetSystemFont (const char *gsettingspath)
{
  return NULL;
}

int
osCreateLink (const char *target, const char *linkpath)
{
  return -1;
}

bool
osIsLink (const char *path)
{
  return false;
}

void *
osToWideChar (const char *buff)
{
  OS_FS_CHAR_TYPE *tbuff = NULL;
  size_t      len;

  /* the documentation lies; len does not include room for the null byte */
  len = MultiByteToWideChar (CP_UTF8, 0, buff, strlen (buff), NULL, 0);
  tbuff = mdmalloc ((len + 1) * OS_FS_CHAR_SIZE);
  MultiByteToWideChar (CP_UTF8, 0, buff, strlen (buff), tbuff, len);
  tbuff [len] = L'\0';
  return tbuff;
}

char *
osFromWideChar (const void *buff)
{
  char        *tbuff = NULL;
  size_t      len;

  /* the documentation lies; len does not include room for the null byte */
  len = WideCharToMultiByte (CP_UTF8, 0, buff, -1, NULL, 0, NULL, NULL);
  tbuff = mdmalloc (len + 1);
  WideCharToMultiByte (CP_UTF8, 0, buff, -1, tbuff, len, NULL, NULL);
  tbuff [len] = '\0';
  return tbuff;
}

void
osSuspendSleep (void)
{
  SetThreadExecutionState (ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
}

void
osResumeSleep (void)
{
  SetThreadExecutionState (ES_CONTINUOUS);
}

#endif /* _WIN32 */
