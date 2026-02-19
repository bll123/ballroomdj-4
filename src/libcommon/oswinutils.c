/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "osutils.h"

void
osRegistryGet (char *key, char *name, char *buff, size_t sz)
{
  char    *trval = NULL;
  HKEY    hkey;
  LSTATUS rc;
  unsigned char tbuff [BDJ4_PATH_MAX * sizeof (wchar_t)];
  DWORD   len = BDJ4_PATH_MAX * sizeof (wchar_t);
  wchar_t *wkey;
  wchar_t *wname;

  *buff = '\0';
  *tbuff = '\0';

  wkey = osToWideChar (key);

  rc = RegOpenKeyExW (
      HKEY_CURRENT_USER,
      wkey,
      0,
      KEY_QUERY_VALUE,
      &hkey
      );
  mdfree (wkey);

  if (rc == ERROR_SUCCESS) {
    wname = osToWideChar (name);

    rc = RegQueryValueExW (
        hkey,
        wname,
        NULL,
        NULL,
        tbuff,
        &len
        );
    if (rc == ERROR_SUCCESS) {
      trval = osFromWideChar ((wchar_t *) tbuff);
    }
    RegCloseKey (hkey);
    mdfree (wname);
  }

  *buff = '\0';
  if (trval != NULL) {
    stpecpy (buff, buff + sz, trval);
  }
}

void
osGetSystemFont (const char *gsettingspath, char *buff, size_t sz)
{
  *buff = '\0';
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

BDJ_NODISCARD
wchar_t *
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

BDJ_NODISCARD
char *
osFromWideChar (const wchar_t *buff)
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
