/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjstring.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "osutils.h"
#include "sysvars.h"

#define ENAME "ABC"
#define EDATA "def123"
#define WTEST "abcÜÄÑÖ내가 제일 잘 나가ははは夕陽伴我歸Ne_Русский_Шторм"

START_TEST(osutils_link)
{
  const char  *fnad = "tmp/osutils.txt";
  const char  *fna = "osutils.txt";
  const char  *fnb = "tmp/link.txt";
  FILE        *fh;
  bool        exists;
  bool        expect;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osutils_link");
  mdebugSubTag ("osutils_link");
  fileopDelete (fnad);
  fileopDelete (fnb);

  fh = fileopOpen (fnad, "w");
  if (fh != NULL) {
    fclose (fh);
  }

  expect = false;
  if (! isWindows ()) {
    expect = true;
  }

  osCreateLink (fna, fnb);
  exists = osIsLink (fnb);
  ck_assert_int_eq (exists, expect);
  exists = fileopFileExists (fnb);
  ck_assert_int_eq (exists, expect);

  fileopDelete (fnad);
  fileopDelete (fnb);
}
END_TEST

START_TEST(osutils_getsysfont)
{
  char  *tptr = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osutils_getsysfont");
  mdebugSubTag ("osutils_getsysfont");
  tptr = osGetSystemFont (sysvarsGetStr (SV_PATH_GSETTINGS));
  if (isLinux ()) {
    ck_assert_ptr_nonnull (tptr);
    ck_assert_str_ne (tptr, "");
    mdfree (tptr);
  } else {
    ck_assert_ptr_null (tptr);
  }
}
END_TEST

START_TEST(osutils_getregistry)
{
  char  *tptr = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osutils_getregistry");
  mdebugSubTag ("osutils_getregistry");
  tptr = osRegistryGet (
      "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
      "My Music");
  if (isWindows ()) {
    ck_assert_ptr_nonnull (tptr);
    ck_assert_str_ne (tptr, "");
  } else {
    ck_assert_ptr_null (tptr);
  }
}
END_TEST

START_TEST(osutils_widechar)
{
  if (isWindows ()) {
#if _WINNT
    wchar_t *wcs;
    char    *nstr;

    logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osutils_widechar");
    mdebugSubTag ("osutils_widechar");
    wcs = osToWideChar (WTEST);
    ck_assert_ptr_nonnull (wcs);
    nstr = osFromWideChar (wcs);
    ck_assert_ptr_nonnull (nstr);
    ck_assert_str_eq (WTEST, nstr);

    mdfree (wcs);
    mdfree (nstr);
#endif
  }
}
END_TEST

Suite *
osutils_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("osutils");
  tc = tcase_create ("osutils");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, osutils_link);
  tcase_add_test (tc, osutils_getsysfont);
  tcase_add_test (tc, osutils_getregistry);
  tcase_add_test (tc, osutils_widechar);
  suite_add_tcase (s, tc);

  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
