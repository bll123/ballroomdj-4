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
#include "sysvars.h"

START_TEST(fileop_exists_a)
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

START_TEST(fileop_size_a)
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

START_TEST(fileop_modtime_a)
{
  FILE      *fh;
  time_t    ctm;
  time_t    tm;

  char *fn = "tmp/def.txt";
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

START_TEST(fileop_delete_a)
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

/* update the fnlist in check_fileop.c and check_dirop.c also */
static char *fnlist [] = {
  "tmp/ÜÄÑÖ",
  "tmp/I Am the Best (내가 제일 잘 나가)",
  "tmp/ははは",
  "tmp/夕陽伴我歸",
  "tmp/Ne_Русский_Шторм",
};
enum {
  fnlistsz = sizeof (fnlist) / sizeof (char *),
};

START_TEST(fileop_open_u)
{
  FILE    *fh;

  for (int i = 0; i < fnlistsz; ++i) {
    char *fn = fnlist [i];
    fh = fileopOpen (fn, "w");
    ck_assert_ptr_nonnull (fh);
    fclose (fh);
  }
}

START_TEST(fileop_exists_u)
{
  FILE    *fh;
  int     rc;

  for (int i = 0; i < fnlistsz; ++i) {
    char *fn = fnlist [i];
    fh = fileopOpen (fn, "w");
    ck_assert_ptr_nonnull (fh);
    fclose (fh);
    rc = fileopFileExists (fn);
    ck_assert_int_eq (rc, 1);
  }
}

START_TEST(fileop_del_u)
{
  FILE    *fh;
  int     rc;

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

START_TEST(fileop_size_u)
{
  FILE      *fh;
  ssize_t   sz;

  for (int i = 0; i < fnlistsz; ++i) {
    char *fn = fnlist [i];
    fh = fileopOpen (fn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "abcdef");
    fclose (fh);
    sz = fileopSize (fn);
    ck_assert_int_eq (sz, 6);
    fileopDelete (fn);
  }
}

START_TEST(fileop_modtime_u)
{
  FILE      *fh;
  time_t    ctm;
  time_t    tm;

  ctm = time (NULL);
  for (int i = 0; i < fnlistsz; ++i) {
    char *fn = fnlist [i];
    fh = fileopOpen (fn, "w");
    ck_assert_ptr_nonnull (fh);
    fclose (fh);
    tm = fileopModTime (fn);
    ck_assert_int_ne (tm, 0);
    ck_assert_int_ge (tm, ctm);
    fileopDelete (fn);
  }
}

Suite *
fileop_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("fileop Suite");
  tc = tcase_create ("fileop");
  tcase_add_test (tc, fileop_exists_a);
  tcase_add_test (tc, fileop_size_a);
  tcase_add_test (tc, fileop_modtime_a);
  tcase_add_test (tc, fileop_delete_a);
  tcase_add_test (tc, fileop_open_u);
  tcase_add_test (tc, fileop_exists_u);
  tcase_add_test (tc, fileop_del_u);
  tcase_add_test (tc, fileop_size_u);
  tcase_add_test (tc, fileop_modtime_u);
  suite_add_tcase (s, tc);
  return s;
}

