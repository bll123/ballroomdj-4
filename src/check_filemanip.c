#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "filemanip.h"
#include "fileop.h"
#include "check_bdj.h"
#include "portability.h"

START_TEST(filemanip_copy)
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
  rc = filemanipCopy (ofn, nfn);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn);
  ck_assert_int_eq (rc, 1);
  rc = fileopExists (nfn);
  ck_assert_int_eq (rc, 1);
  unlink (ofn);
  unlink (nfn);
}
END_TEST

START_TEST(filemanip_move)
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
  rc = filemanipMove (ofn, nfn);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (nfn);
  ck_assert_int_eq (rc, 1);
  unlink (ofn);
  unlink (nfn);
}
END_TEST


START_TEST(filemanip_del_dir)
{
  FILE      *fh;
  int       rc;

  /* note that this fails on windows running under msys2 */
  char *dfn = "tmp/abc";
  char *ofn = "tmp/abc/def";
  char *nfn = "tmp/abc/def/xyz.txt";
  fileopMakeDir (dfn);
  fileopMakeDir (ofn);
  fh = fopen (nfn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = fileopExists (dfn);
  ck_assert_int_eq (rc, 1);
  rc = fileopIsDirectory (dfn);
  ck_assert_int_eq (rc, 1);
  rc = fileopExists (dfn);
  ck_assert_int_eq (rc, 1);
  rc = fileopIsDirectory (ofn);
  ck_assert_int_eq (rc, 1);
  rc = fileopExists (nfn);
  ck_assert_int_eq (rc, 1);
  filemanipDeleteDir (dfn);
  rc = fileopExists (nfn);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (dfn);
  ck_assert_int_eq (rc, 0);
}
END_TEST


Suite *
filemanip_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("File Manip Suite");
  tc = tcase_create ("File Manip");
  tcase_add_test (tc, filemanip_copy);
  tcase_add_test (tc, filemanip_move);
  tcase_add_test (tc, filemanip_del_dir);
  suite_add_tcase (s, tc);
  return s;
}

