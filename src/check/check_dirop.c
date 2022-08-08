#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "dirop.h"
#include "filemanip.h"
#include "fileop.h"
#include "check_bdj.h"
#include "slist.h"
#include "sysvars.h"

START_TEST(dirop_del_dir)
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

  diropDeleteDir (dfn);
  rc = fileopFileExists (nfn);
  ck_assert_int_eq (rc, 0);
  rc = fileopIsDirectory (ofn);
  ck_assert_int_eq (rc, 0);
  rc = fileopIsDirectory (dfn);
  ck_assert_int_eq (rc, 0);
}
END_TEST

START_TEST(dirop_basic_dirlist)
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

  slist = diropBasicDirList (dafn, NULL);

  ck_assert_int_eq (slistGetCount (slist), 1);
  slistStartIterator (slist, &iteridx);
  while ((fn = slistIterateKey (slist, &iteridx)) != NULL) {
    ck_assert_str_eq (fn, "abc.txt");
  }
  slistFree (slist);

  slist = diropBasicDirList (dcfn, NULL);
  ck_assert_int_eq (slistGetCount (slist), 1);
  slistStartIterator (slist, &iteridx);
  while ((fn = slistIterateKey (slist, &iteridx)) != NULL) {
    ck_assert_str_eq (fn, "ghi.txt");
  }
  slistFree (slist);

  diropDeleteDir (dafn);
}
END_TEST


START_TEST(dirop_recursive_dirlist)
{
  FILE      *fh;
  slist_t   *slist;
  slistidx_t  iteridx;
  char      *fn;
  int       num = 3;

  char *dafn = "tmp/abc";
  char *fafn = "tmp/abc/abc.txt";
  char *dbfn = "tmp/abc/def";
  char *fbfn = "tmp/abc/def/def.txt";
  char *dcfn = "tmp/abc/ghi";
  char *ddfn = "tmp/abc/jkl";
  char *fcfn = "tmp/abc/ghi/ghi.txt";
  char *fdfn = "tmp/abc/ghi/jkl.txt";

  sysvarsInit ("check_filemanip");

  diropDeleteDir (dafn);

  fileopMakeDir (dbfn);
  fileopMakeDir (dcfn);
  if (! isWindows ()) {
    filemanipLinkCopy ("ghi", ddfn);
    filemanipLinkCopy ("ghi.txt", fdfn);
    num = 6;
  }
  fh = fopen (fafn, "w");
  fclose (fh);
  fh = fopen (fbfn, "w");
  fclose (fh);
  fh = fopen (fcfn, "w");
  fclose (fh);

  slist = diropRecursiveDirList (dafn, FILEMANIP_FILES);


  ck_assert_int_eq (slistGetCount (slist), num);
  /* the list is unordered; for checks, sort it */
  slistSort (slist);
  slistStartIterator (slist, &iteridx);
  fn = slistIterateKey (slist, &iteridx);
  ck_assert_str_eq (fn, "tmp/abc/abc.txt");
  fn = slistIterateKey (slist, &iteridx);
  ck_assert_str_eq (fn, "tmp/abc/def/def.txt");
  fn = slistIterateKey (slist, &iteridx);
  ck_assert_str_eq (fn, "tmp/abc/ghi/ghi.txt");
  if (! isWindows ()) {
    fn = slistIterateKey (slist, &iteridx);
    ck_assert_str_eq (fn, "tmp/abc/ghi/jkl.txt");
    fn = slistIterateKey (slist, &iteridx);
    ck_assert_str_eq (fn, "tmp/abc/jkl/ghi.txt");
    fn = slistIterateKey (slist, &iteridx);
    ck_assert_str_eq (fn, "tmp/abc/jkl/jkl.txt");
  }

  diropDeleteDir (dafn);
  slistFree (slist);
}
END_TEST


Suite *
dirop_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Directory Op Suite");
  tc = tcase_create ("Directory Op");
  tcase_add_test (tc, dirop_del_dir);
  tcase_add_test (tc, dirop_basic_dirlist);
  tcase_add_test (tc, dirop_recursive_dirlist);
  suite_add_tcase (s, tc);
  return s;
}

