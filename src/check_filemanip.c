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
#include "slist.h"

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
  rc = fileopFileExists (ofn);
  ck_assert_int_eq (rc, 1);
  rc = fileopFileExists (nfn);
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
  rc = fileopFileExists (ofn);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (nfn);
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
  rc = fileopMakeDir (dfn);
  ck_assert_int_eq (rc, 0);
  rc = fileopMakeDir (ofn);
  ck_assert_int_eq (rc, 0);
  fh = fopen (nfn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = fileopIsDirectory (dfn);
  ck_assert_int_eq (rc, 1);
  rc = fileopIsDirectory (ofn);
  ck_assert_int_eq (rc, 1);
  rc = fileopFileExists (nfn);
  ck_assert_int_eq (rc, 1);

  filemanipDeleteDir (dfn);
  rc = fileopFileExists (nfn);
  ck_assert_int_eq (rc, 0);
  rc = fileopIsDirectory (ofn);
  ck_assert_int_eq (rc, 0);
  rc = fileopIsDirectory (dfn);
  ck_assert_int_eq (rc, 0);
}
END_TEST

START_TEST(filemanip_basic_dirlist)
{
  FILE      *fh;
  slist_t   *slist;
  slistidx_t  iteridx;
  char      *fn;

  char *dafn = "tmp/abc";
  char *fafn = "tmp/abc/abc.txt";
  char *dbfn = "tmp/abc/def";
  char *fbfn = "tmp/abc/def/def.txt";
  char *dcfn = "tmp/abc/ghi";
  char *fcfn = "tmp/abc/ghi/ghi.txt";
  fileopMakeDir (dbfn);
  fileopMakeDir (dcfn);
  fh = fopen (fafn, "w");
  fclose (fh);
  fh = fopen (fbfn, "w");
  fclose (fh);
  fh = fopen (fcfn, "w");
  fclose (fh);

  slist = filemanipBasicDirList (dafn, NULL);

  ck_assert_int_eq (slistGetCount (slist), 1);
  slistStartIterator (slist, &iteridx);
  while ((fn = slistIterateKey (slist, &iteridx)) != NULL) {
    ck_assert_str_eq (fn, "abc.txt");
  }

  slist = filemanipBasicDirList (dcfn, NULL);
  ck_assert_int_eq (slistGetCount (slist), 1);
  slistStartIterator (slist, &iteridx);
  while ((fn = slistIterateKey (slist, &iteridx)) != NULL) {
    ck_assert_str_eq (fn, "ghi.txt");
  }

  filemanipDeleteDir (dafn);
}
END_TEST


START_TEST(filemanip_recursive_dirlist)
{
  FILE      *fh;
  slist_t   *slist;
  slistidx_t  iteridx;
  char      *fn;

  char *dafn = "tmp/abc";
  char *fafn = "tmp/abc/abc.txt";
  char *dbfn = "tmp/abc/def";
  char *fbfn = "tmp/abc/def/def.txt";
  char *dcfn = "tmp/abc/ghi";
  char *fcfn = "tmp/abc/ghi/ghi.txt";
  fileopMakeDir (dbfn);
  fileopMakeDir (dcfn);
  fh = fopen (fafn, "w");
  fclose (fh);
  fh = fopen (fbfn, "w");
  fclose (fh);
  fh = fopen (fcfn, "w");
  fclose (fh);

  slist = filemanipRecursiveDirList (dafn);

  ck_assert_int_eq (slistGetCount (slist), 3);
  /* the list is unordered; for checks, sort it */
  slistSort (slist);
  slistStartIterator (slist, &iteridx);
  fn = slistIterateKey (slist, &iteridx);
  ck_assert_str_eq (fn, "tmp/abc/abc.txt");
  fn = slistIterateKey (slist, &iteridx);
  ck_assert_str_eq (fn, "tmp/abc/def/def.txt");
  fn = slistIterateKey (slist, &iteridx);
  ck_assert_str_eq (fn, "tmp/abc/ghi/ghi.txt");

  filemanipDeleteDir (dafn);
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
  tcase_add_test (tc, filemanip_basic_dirlist);
  tcase_add_test (tc, filemanip_recursive_dirlist);
  suite_add_tcase (s, tc);
  return s;
}

