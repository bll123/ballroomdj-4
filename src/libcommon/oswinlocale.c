/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

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

#endif /* _WIN32 */
