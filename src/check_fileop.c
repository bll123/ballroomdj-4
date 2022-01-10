#include "config.h"
#include "configt.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>
#if _hdr_unistd
# include <unistd.h>
#endif

#include "fileop.h"
#include "check_bdj.h"
#include "portability.h"

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
  rc = fileopExists (fn);
  ck_assert_int_ne (rc, 0);
  rc = fileopExists ("tmp/def.txt");
  ck_assert_int_eq (rc, 0);
  unlink (fn);
}
END_TEST

START_TEST(fileop_copy)
{
  FILE      *fh;
  int       rc;

  char *ofn = "tmp/abc.txt.a";
  char *nfn = "tmp/abc.txt.b";
  unlink (ofn);
  unlink (nfn);
  fh = fopen (ofn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = fileopCopy (ofn, nfn);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopExists (nfn);
  ck_assert_int_ne (rc, 0);
  unlink (ofn);
  unlink (nfn);
}
END_TEST

START_TEST(fileop_move)
{
  FILE      *fh;
  int       rc;

  char *ofn = "tmp/abc.txt.c";
  char *nfn = "tmp/abc.txt.d";
  unlink (ofn);
  unlink (nfn);
  fh = fopen (ofn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = fileopMove (ofn, nfn);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (nfn);
  ck_assert_int_ne (rc, 0);
  unlink (ofn);
  unlink (nfn);
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

Suite *
fileop_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("File Ops Suite");
  tc = tcase_create ("File Op");
  tcase_add_test (tc, fileop_exists);
  tcase_add_test (tc, fileop_copy);
  tcase_add_test (tc, fileop_move);
  tcase_add_test (tc, fileop_delete);
  suite_add_tcase (s, tc);
  return s;
}

