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
#include <sys/stat.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "mdebug.h"
#include "fileop.h"
#include "log.h"
#include "osutils.h"
#include "sysvars.h"

START_TEST(fileop_exists_a)
{
  FILE      *fh;
  int       rc;
  char *fn = "tmp/def.txt";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_exists_a");
  mdebugSubTag ("fileop_exists_a");

  unlink (fn);
  fn = "tmp/abc.txt";
  fh = fopen (fn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = fileopFileExists (fn);
  ck_assert_int_eq (rc, 1);
  rc = fileopFileExists ("tmp/def.txt");
  ck_assert_int_eq (rc, 0);
  unlink (fn);
}
END_TEST

START_TEST(fileop_exists_symlink)
{
  FILE  *fh;
  int   rc;
  char  *fn  = "tmp/slabc.txt";
  char  *fnb = "tmp/sldef.txt";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_exists_symlink");
  mdebugSubTag ("fileop_exists_symlink");

  unlink (fn);
  unlink (fnb);

  fh = fopen (fn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = osCreateLink ("slabc.txt", fnb);
  rc = fileopFileExists (fn);
  ck_assert_int_eq (rc, 1);
  if (! isWindows ()) {
    rc = fileopFileExists (fnb);
    ck_assert_int_eq (rc, 1);
    rc = osIsLink (fnb);
    ck_assert_int_eq (rc, 1);
  }
  rc = fileopFileExists ("tmp/slghi.txt");
  ck_assert_int_eq (rc, 0);

  unlink (fn);

  if (! isWindows ()) {
    /* the symlink exists though the file does not */
    /* fileopFileExists will return false, as it uses stat() */
    rc = fileopFileExists (fnb);
    ck_assert_int_eq (rc, 0);
    /* for the time being, check this here */
    rc = osIsLink (fnb);
    ck_assert_int_eq (rc, 1);
  }
  unlink (fnb);
}
END_TEST

START_TEST(fileop_size_a)
{
  FILE      *fh;
  ssize_t   sz;
  char *fn = "tmp/def.txt";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_size_a");
  mdebugSubTag ("fileop_size_a");

  unlink (fn);
  fn = "tmp/abc.txt";
  fh = fopen (fn, "w");
  fprintf (fh, "abcdef");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  sz = fileopSize (fn);
  ck_assert_int_eq (sz, 6);
  sz = fileopSize ("tmp/def.txt");
  ck_assert_int_eq (sz, -1);
  unlink (fn);
}
END_TEST

START_TEST(fileop_modtime_a)
{
  FILE      *fh;
  time_t    ctm;
  time_t    tm;
  char *fn = "tmp/def.txt";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_modtime_a");
  mdebugSubTag ("fileop_modtime_a");

  ctm = time (NULL);
  unlink (fn);
  fn = "tmp/abc.txt";
  fh = fopen (fn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  tm = fileopModTime (fn);
  ck_assert_int_ne (tm, 0);
  ck_assert_int_ge (tm, ctm);
  unlink (fn);
}
END_TEST

START_TEST(fileop_setmodtime_a)
{
  FILE      *fh;
  time_t    ctm;
  time_t    tm;
  char *fn = "tmp/def.txt";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_setmodtime_a");
  mdebugSubTag ("fileop_setmodtime_a");

  ctm = time (NULL);
  unlink (fn);
  fn = "tmp/abc.txt";
  fh = fopen (fn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  tm = fileopModTime (fn);
  ck_assert_int_ne (tm, 0);
  ck_assert_int_ge (tm, ctm);
  fileopSetModTime (fn, 28800 * 2 + 12);
  tm = fileopModTime (fn);
  ck_assert_int_ne (tm, 0);
  ck_assert_int_eq (tm, 28800 * 2 + 12);
  unlink (fn);
}
END_TEST

START_TEST(fileop_delete_a)
{
  FILE      *fh;
  int       rc;
  char *fn = "tmp/def.txt";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_delete_a");
  mdebugSubTag ("fileop_delete_a");

  unlink (fn);
  fn = "tmp/abc.txt";
  fh = fopen (fn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = fileopFileExists (fn);
  ck_assert_int_eq (rc, 1);
  rc = fileopDelete (fn);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (fn);
  ck_assert_int_eq (rc, 0);
  rc = fileopDelete ("tmp/def.txt");
  ck_assert_int_lt (rc, 0);
  unlink (fn);
}
END_TEST

START_TEST(fileop_delete_symlink)
{
  FILE  *fh;
  int   rc;
  char  *fn = "tmp/slghi.txt";
  char  *fnb = "tmp/sljkl.txt";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_delete_symlink");
  mdebugSubTag ("fileop_delete_symlink");

  unlink (fn);
  fh = fopen (fn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = osCreateLink ("slghi.txt", fnb);

  rc = fileopFileExists (fn);
  ck_assert_int_eq (rc, 1);
  if (! isWindows ()) {
    rc = fileopFileExists (fnb);
    ck_assert_int_eq (rc, 1);
    rc = fileopDelete (fnb);
    ck_assert_int_eq (rc, 0);
  }
  rc = fileopFileExists (fn);
  ck_assert_int_eq (rc, 1);
  if (! isWindows ()) {
    rc = fileopFileExists (fnb);
    ck_assert_int_eq (rc, 0);
  }

  rc = osCreateLink ("slghi.txt", fnb);
  if (! isWindows ()) {
    rc = fileopFileExists (fnb);
    ck_assert_int_eq (rc, 1);
  }

  rc = fileopDelete (fn);
  ck_assert_int_eq (rc, 0);

  rc = fileopFileExists (fn);
  ck_assert_int_eq (rc, 0);
  if (! isWindows ()) {
    rc = fileopFileExists (fnb);
    ck_assert_int_eq (rc, 0);
    rc = osIsLink (fnb);
    ck_assert_int_eq (rc, 1);
    rc = fileopDelete (fnb);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (fnb);
    ck_assert_int_eq (rc, 0);
    rc = osIsLink (fnb);
    ck_assert_int_eq (rc, 0);
  }

  rc = fileopDelete ("tmp/slmno.txt");
  ck_assert_int_lt (rc, 0);
  unlink (fn);
  unlink (fnb);
}
END_TEST

/* update the fnlist in fileop/filemanip/dirop/dirlist also */
static char *fnlist [] = {
  "tmp/abc-def.txt",
  "tmp/ÜÄÑÖ.txt",
  "tmp/I Am the Best_내가 제일 잘 나가.txt",
  "tmp/ははは.txt",
  "tmp/夕陽伴我歸.txt",
  "tmp/Ne_Русский_Шторм.txt",
};
enum {
  fnlistsz = sizeof (fnlist) / sizeof (char *),
};

START_TEST(fileop_open_u)
{
  FILE    *fh;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_open_u");
  mdebugSubTag ("fileop_open_u");

  for (int i = 0; i < fnlistsz; ++i) {
    char *fn = fnlist [i];
    fh = fileopOpen (fn, "w");
    ck_assert_ptr_nonnull (fh);
    fclose (fh);
  }
}
END_TEST

START_TEST(fileop_exists_u)
{
  FILE    *fh;
  int     rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_exists_u");
  mdebugSubTag ("fileop_exists_u");

  for (int i = 0; i < fnlistsz; ++i) {
    char *fn = fnlist [i];
    fh = fileopOpen (fn, "w");
    ck_assert_ptr_nonnull (fh);
    fclose (fh);
    rc = fileopFileExists (fn);
    ck_assert_int_eq (rc, 1);
  }
}
END_TEST

START_TEST(fileop_del_u)
{
  FILE    *fh;
  int     rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_del_u");
  mdebugSubTag ("fileop_del_u");

  for (int i = 0; i < fnlistsz; ++i) {
    char *fn = fnlist [i];
    fh = fileopOpen (fn, "w");
    ck_assert_ptr_nonnull (fh);
    fclose (fh);
    rc = fileopFileExists (fn);
    ck_assert_int_eq (rc, 1);
    fileopDelete (fn);
    rc = fileopFileExists (fn);
    ck_assert_int_eq (rc, 0);
  }
}
END_TEST

START_TEST(fileop_size_u)
{
  FILE      *fh;
  ssize_t   sz;
  int       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_size_u");
  mdebugSubTag ("fileop_size_u");

  for (int i = 0; i < fnlistsz; ++i) {
    char *fn = fnlist [i];
    fh = fileopOpen (fn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "abcdef");
    fclose (fh);
    rc = fileopFileExists (fn);
    ck_assert_int_eq (rc, 1);
    sz = fileopSize (fn);
    ck_assert_int_eq (sz, 6);
    fileopDelete (fn);
  }
}
END_TEST

START_TEST(fileop_modtime_u)
{
  FILE      *fh;
  time_t    ctm;
  time_t    tm;
  int       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_modtime_u");
  mdebugSubTag ("fileop_modtime_u");

  ctm = time (NULL);
  for (int i = 0; i < fnlistsz; ++i) {
    char *fn = fnlist [i];
    fh = fileopOpen (fn, "w");
    ck_assert_ptr_nonnull (fh);
    fclose (fh);
    rc = fileopFileExists (fn);
    ck_assert_int_eq (rc, 1);
    tm = fileopModTime (fn);
    ck_assert_int_ne (tm, 0);
    ck_assert_int_ge (tm, ctm);
    fileopDelete (fn);
  }
}
END_TEST

START_TEST(fileop_setmodtime_u)
{
  FILE      *fh;
  time_t    ctm;
  time_t    tm;
  int       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_setmodtime_u");
  mdebugSubTag ("fileop_setmodtime_u");

  ctm = time (NULL);
  for (int i = 0; i < fnlistsz; ++i) {
    char *fn = fnlist [i];
    fh = fileopOpen (fn, "w");
    ck_assert_ptr_nonnull (fh);
    fclose (fh);
    rc = fileopFileExists (fn);
    ck_assert_int_eq (rc, 1);
    tm = fileopModTime (fn);
    ck_assert_int_ne (tm, 0);
    ck_assert_int_ge (tm, ctm);
    fileopSetModTime (fn, 28800 * 2 + 14);
    tm = fileopModTime (fn);
    ck_assert_int_ne (tm, 0);
    ck_assert_int_eq (tm, 28800 * 2 + 14);
    fileopDelete (fn);
  }
}
END_TEST

Suite *
fileop_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("fileop");
  tc = tcase_create ("fileop");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, fileop_exists_a);
  tcase_add_test (tc, fileop_exists_symlink);
  tcase_add_test (tc, fileop_size_a);
  tcase_add_test (tc, fileop_modtime_a);
  tcase_add_test (tc, fileop_setmodtime_a);
  tcase_add_test (tc, fileop_delete_a);
  tcase_add_test (tc, fileop_delete_symlink);
  tcase_add_test (tc, fileop_open_u);
  tcase_add_test (tc, fileop_exists_u);
  tcase_add_test (tc, fileop_del_u);
  tcase_add_test (tc, fileop_size_u);
  tcase_add_test (tc, fileop_modtime_u);
  tcase_add_test (tc, fileop_setmodtime_u);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
