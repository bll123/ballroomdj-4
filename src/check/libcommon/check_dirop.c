/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

#include "bdj4.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "dirlist.h"
#include "dirop.h"
#include "filemanip.h"
#include "fileop.h"
#include "log.h"
#include "slist.h"
#include "sysvars.h"

/* update the fnlist in fileop/filemanip/dirop/dirlist also */
static const char *fnlist [] = {
  "tmp/abc-def",
  "tmp/def",
  "tmp/def/ghi",
  "tmp/jkl",
  "tmp/jkl/abc",
  "tmp/jkl/def",
  "tmp/jkl/ghi",
  "tmp/ÜÄÑÖ",
  "tmp/I Am the Best_내가 제일 잘 나가",
  "tmp/ははは",
  "tmp/夕陽伴我歸",
  "tmp/Ne_Русский_Шторм",
};
enum {
  fnlistsz = sizeof (fnlist) / sizeof (char *),
};

START_TEST(dirop_make)
{
  int       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dirop_mkdir_isdir_a");
  mdebugSubTag ("dirop_mkdir_isdir_a");

  for (int i = 0; i < fnlistsz; ++i) {
    const char *fn = fnlist [i];

    fileopDelete (fn);
    diropDeleteDir (fn, DIROP_ALL);

    rc = diropMakeDir (fn);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (fn);
    ck_assert_int_eq (rc, 0);
    rc = fileopIsDirectory (fn);
    ck_assert_int_eq (rc, 1);
  }
}
END_TEST

START_TEST(dirop_del)
{
  int       rc;
  char      tbuff [BDJ4_PATH_MAX];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dirop_del");
  mdebugSubTag ("dirop_del");

  for (int i = 0; i < fnlistsz; ++i) {
    const char  *fn = fnlist [i];
    FILE        *fh;

    fileopDelete (fn);
    diropDeleteDir (fn, DIROP_ALL);
    rc = diropMakeDir (fn);
    ck_assert_int_eq (rc, 0);

    snprintf (tbuff, sizeof (tbuff), "%s/abc.txt", fn);
    fh = fileopOpen (tbuff, "w");
    ck_assert_ptr_nonnull (fh);
    mdextfclose (fh);
    fclose (fh);
    rc = fileopFileExists (tbuff);
    ck_assert_int_eq (rc, true);

    diropDeleteDir (fn, DIROP_ALL);
    rc = fileopIsDirectory (fn);
    ck_assert_int_eq (rc, false);
  }
}
END_TEST

START_TEST(dirop_del_not_empty)
{
  int     rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dirop_del_not_empty");
  mdebugSubTag ("dirop_del_not_empty");

  for (int i = 0; i < fnlistsz; ++i) {
    const char *fn = fnlist [i];

    fileopDelete (fn);
    diropDeleteDir (fn, DIROP_ALL);

    rc = diropMakeDir (fn);
  }

  for (int i = 0; i < fnlistsz; ++i) {
    const char  *fn = fnlist [i];
    FILE        *fh;
    char        tbuff [BDJ4_PATH_MAX];

    if (i == 0 || i == 5 || i == 6 || i == 8) {
      snprintf (tbuff, sizeof (tbuff), "%s/abc.txt", fn);
      fh = fileopOpen (tbuff, "w");
      ck_assert_ptr_nonnull (fh);
      mdextfclose (fh);
      fclose (fh);
      rc = fileopFileExists (tbuff);
      ck_assert_int_eq (rc, true);
    }
  }

  for (int i = 0; i < fnlistsz; ++i) {
    const char  *fn = fnlist [i];
    bool        ret;
    char        tbuff [BDJ4_PATH_MAX];

    ret = diropDeleteDir (fn, DIROP_ONLY_IF_EMPTY);
    rc = fileopIsDirectory (fn);
    if (i == 0 || i == 5 || i == 6 || i == 8) {
      ck_assert_int_eq (ret, false);
      ck_assert_int_eq (rc, true);
      snprintf (tbuff, sizeof (tbuff), "%s/abc.txt", fn);
      rc = fileopFileExists (tbuff);
      ck_assert_int_eq (rc, true);
    } else if (i == 3) {
      /* top level directory, files exist underneath */
      ck_assert_int_eq (ret, false);
      ck_assert_int_eq (rc, true);
    } else if (i == 2) {
      /* this directory doesn't exist, as the dir over it was removed */
      ck_assert_int_eq (ret, false);
      ck_assert_int_eq (rc, false);
    } else if (i == 4) {
      /* indeterminate */
    } else {
      /* empty directories */
      ck_assert_int_eq (ret, true);
      ck_assert_int_eq (rc, false);
    }
  }

  for (int i = 0; i < fnlistsz; ++i) {
    const char *fn = fnlist [i];

    diropDeleteDir (fn, DIROP_ALL);
  }
}
END_TEST

Suite *
dirop_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("dirop");
  tc = tcase_create ("dirop");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, dirop_make);
  tcase_add_test (tc, dirop_del);
  tcase_add_test (tc, dirop_del_not_empty);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
