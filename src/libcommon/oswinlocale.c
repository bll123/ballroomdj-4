/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wchar.h>

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include "bdj4.h"
#include "mdebug.h"
#include "oslocale.h"
#include "osutils.h"

int
osLocaleDirection (const char *locale)
{
  int       tdir = TEXT_DIR_LTR;
#if _lib_GetLocaleInfoEx
  wchar_t   *wlocale;
  DWORD     value;

  wlocale = osToWideChar (locale);
  GetLocaleInfoEx (wlocale, LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER,
      (LPWSTR) &value, sizeof (value) / sizeof (wchar_t));
  if (value == 1) {
    tdir = TEXT_DIR_RTL;
  }
  dataFree (wlocale);
#endif

  return tdir;
}

void
osGetPreferredLocales (char *buff, size_t sz)
{
  /* windows does not return a list of languages.  */
  /* the current get-os-locale method is fine. */
  *buff = '\0';
}
