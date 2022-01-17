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
  tcase_add_test (tc, fileop_delete);
  suite_add_tcase (s, tc);
  return s;
}

