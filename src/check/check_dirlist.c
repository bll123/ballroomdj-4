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

#include "dirlist.h"
#include "dirop.h"
#include "filemanip.h"
#include "fileop.h"
#include "check_bdj.h"
#include "slist.h"
#include "sysvars.h"

START_TEST(dirlist_basic)
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
  diropMakeDir (dbfn);
  diropMakeDir (dcfn);
  fh = fileopOpen (fafn, "w");
  fclose (fh);
  fh = fileopOpen (fbfn, "w");
  fclose (fh);
  fh = fileopOpen (fcfn, "w");
  fclose (fh);

  slist = dirlistBasicDirList (dafn, NULL);

  ck_assert_int_eq (slistGetCount (slist), 1);
  slistStartIterator (slist, &iteridx);
  while ((fn = slistIterateKey (slist, &iteridx)) != NULL) {
    ck_assert_str_eq (fn, "abc.txt");
  }
  slistFree (slist);

  slist = dirlistBasicDirList (dcfn, NULL);
  ck_assert_int_eq (slistGetCount (slist), 1);
  slistStartIterator (slist, &iteridx);
  while ((fn = slistIterateKey (slist, &iteridx)) != NULL) {
    ck_assert_str_eq (fn, "ghi.txt");
  }
  slistFree (slist);

  diropDeleteDir (dafn);
}
END_TEST

START_TEST(dirlist_recursive)
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

  diropDeleteDir (dafn);

  diropMakeDir (dbfn);
  diropMakeDir (dcfn);
  if (! isWindows ()) {
    filemanipLinkCopy ("ghi", ddfn);
    filemanipLinkCopy ("ghi.txt", fdfn);
    num = 6;
  }
  fh = fileopOpen (fafn, "w");
  fclose (fh);
  fh = fileopOpen (fbfn, "w");
  fclose (fh);
  fh = fileopOpen (fcfn, "w");
  fclose (fh);

  slist = dirlistRecursiveDirList (dafn, FILEMANIP_FILES);


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

/* update the fnlist in fileop/filemanip/dirop/dirlist also */
static char *fnlist [] = {
  "tmp/abc-def",
  "tmp/ÜÄÑÖ",
  "tmp/I Am the Best_내가 제일 잘 나가",
  "tmp/ははは",
  "tmp/夕陽伴我歸",
  "tmp/Ne_Русский_Шторм",
};
enum {
  fnlistsz = sizeof (fnlist) / sizeof (char *),
};

Suite *
dirlist_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("dirlist Suite");
  tc = tcase_create ("dirlist");
  tcase_add_test (tc, dirlist_basic);
  tcase_add_test (tc, dirlist_recursive);
  suite_add_tcase (s, tc);
  return s;
}

