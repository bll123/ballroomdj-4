/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#if _hdr_windows
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "oslocale.h"
#include "osutils.h"

/* windows setlocale() returns the old style long locale name */
char *
osGetLocale (char *buff, size_t sz)
{
#if _lib_GetUserDefaultUILanguage
  wchar_t locbuff [LOCALE_NAME_MAX_LENGTH];
  char    *tbuff;
  long    langid;

  langid = GetUserDefaultUILanguage ();
  LCIDToLocaleName ((LCID) langid, locbuff, LOCALE_NAME_MAX_LENGTH,
      LOCALE_ALLOW_NEUTRAL_NAMES);
  tbuff = osFromWideChar (locbuff);
  stpecpy (buff, buff + sz, tbuff);
  mdfree (tbuff);
#else
  stpecpy (buff, buff + sz, setlocale (LC_MESSAGES, NULL));
#endif
  return buff;
}

