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
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "fileop.h"
#include "check_bdj.h"

START_TEST(fileop_exists)
{
  FILE      *fh;
  int       rc;

  char *fn = "tmp/def.txt";
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

START_TEST(fileop_size)
{
  FILE      *fh;
  ssize_t   sz;

  char *fn = "tmp/def.txt";
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

START_TEST(fileop_modtime)
{
  FILE      *fh;
  time_t    ctm;
  time_t    tm;

  ctm = time (NULL);
  char *fn = "tmp/def.txt";
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

START_TEST(fileop_delete)
{
  FILE      *fh;
  int       rc;

  char *fn = "tmp/def.txt";
  unlink (fn);
  fn = "tmp/abc.txt";
  fh = fopen (fn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = fileopDelete (fn);
  ck_assert_int_eq (rc, 0);
  rc = fileopDelete ("tmp/def.txt");
  ck_assert_int_lt (rc, 0);
  unlink (fn);
}
END_TEST

START_TEST(fileop_mkdir_isdir)
{
  int       rc;

  char *fn = "tmp/def";
  unlink (fn);
  rmdir (fn);
  fn = "tmp/abc/def";
  rc = fileopMakeDir (fn);
  ck_assert_int_eq (rc, 0);
  fn = "tmp/abc/xyz";
  rc = fileopMakeDir (fn);
  ck_assert_int_eq (rc, 0);

  rc = fileopIsDirectory ("tmp/def");
  ck_assert_int_eq (rc, 0);

  rc = fileopFileExists ("tmp/def");
  ck_assert_int_eq (rc, 0);

  rc = fileopIsDirectory ("tmp/abc");
  ck_assert_int_eq (rc, 1);

  rc = fileopFileExists ("tmp/abc");
  ck_assert_int_eq (rc, 0);

  rc = fileopIsDirectory (fn);
  ck_assert_int_eq (rc, 1);
  /* exists will return false on a directory */
  rc = fileopFileExists ("fn");
  ck_assert_int_eq (rc, 0);

  rc = fileopIsDirectory ("tmp/abc/xyz");
  ck_assert_int_eq (rc, 1);

  rmdir (fn);
  rmdir ("tmp/abc/xyz");
  rmdir ("tmp/abc");
}
END_TEST

START_TEST(fileop_open)
{
  FILE    *fh;
  int     rc;

  char *fn = "tmp/ÜÄÑÖ";
  fh = fileopOpen (fn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  fileopDelete (fn);
  rc = fileopFileExists (fn);
  ck_assert_int_eq (rc, 0);
}

Suite *
fileop_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("File Ops Suite");
  tc = tcase_create ("File Op");
  tcase_add_test (tc, fileop_exists);
  tcase_add_test (tc, fileop_size);
  tcase_add_test (tc, fileop_modtime);
  tcase_add_test (tc, fileop_delete);
  tcase_add_test (tc, fileop_mkdir_isdir);
  tcase_add_test (tc, fileop_open);
  suite_add_tcase (s, tc);
  return s;
}

