#include "config.h"

#if __WINNT__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <windows.h>

char *
osRegistryGet (char *key, char *name)
{
  char    *rval = NULL;
  DWORD   dwRet;
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

  dwRet = RegQueryValueEx (
      hkey,
      name,
      NULL,
      NULL,
      buff,
      &len
      );

  rval = strdup ((char *) buff);

  RegCloseKey (hkey);

  return rval;
}

char *
osGetSystemFont (const char *gsettingspath)
{
  return NULL;
}

#endif
