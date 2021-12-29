#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>
#if _hdr_unistd
# include <unistd.h>
#endif

#include "fileutil.h"
#include "check_bdj.h"
#include "portability.h"

typedef struct {
  char    *path;
  size_t  dlen;
  size_t  flen;
  size_t  blen;
  size_t  elen;
} ftest_t;

static ftest_t tests [] = {
  { "/usr/binx", 4, 4, 4, 0 },
  { "/usr", 1, 3, 3, 0 },
  { "/home/bll/stuff.txt", 9, 9, 5, 3 },
  { "/home/bll/stuff/", 9, 5, 5, 0 },
  { "/home/bll/stuff", 9, 5, 5, 0 },
  { "/home/bll/stuff.x", 9, 7, 5, 1 },
  { "/home/bll/x.stuff", 9, 7, 1, 5 },
  { "/home/bll/stuff.x/", 9, 7, 7, 0 },
  { "/", 1, 1, 1, 0 },
};
#define TCOUNT (sizeof(tests)/sizeof (ftest_t))

START_TEST(fileinfo_chk)
{
  fileinfo_t    *fi;
  char          tfmt [40];

  for (size_t i = 0; i < TCOUNT; ++i) {
    fi = fileInfo (tests[i].path);
    sprintf (tfmt, "%s: i:%s %s/%s", "dlem", SIZE_FMT, SIZE_FMT, SIZE_FMT);
    ck_assert_msg (fi->dlen == tests[i].dlen,
        tfmt, i, fi->dlen, tests[i].dlen);
    sprintf (tfmt, "%s: i:%s %s/%s", "blem", SIZE_FMT, SIZE_FMT, SIZE_FMT);
    ck_assert_msg (fi->blen == tests[i].blen,
        tfmt, i, fi->blen, tests[i].blen);
    sprintf (tfmt, "%s: i:%s %s/%s", "flem", SIZE_FMT, SIZE_FMT, SIZE_FMT);
    ck_assert_msg (fi->flen == tests[i].flen,
        tfmt, i, fi->flen, tests[i].flen);
    sprintf (tfmt, "%s: i:%s %s/%s", "elem", SIZE_FMT, SIZE_FMT, SIZE_FMT);
    ck_assert_msg (fi->elen == tests[i].elen,
        tfmt, i, fi->elen, tests[i].elen);
    fileInfoFree (fi);
  }
}
END_TEST

START_TEST(file_exists)
{
  FILE      *fh;
  int       rc;

  char *fn = "tmp/def.txt";
  unlink (fn);
  fn = "tmp/abc.txt";
  fh = fopen (fn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = fileExists (fn);
  ck_assert_int_ne (rc, 0);
  rc = fileExists ("tmp/def.txt");
  ck_assert_int_eq (rc, 0);
  unlink (fn);
}
END_TEST

START_TEST(file_copy)
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
  rc = fileCopy (ofn, nfn);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileExists (nfn);
  ck_assert_int_ne (rc, 0);
  unlink (ofn);
  unlink (nfn);
}
END_TEST

START_TEST(file_move)
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
  rc = fileMove (ofn, nfn);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (nfn);
  ck_assert_int_ne (rc, 0);
  unlink (ofn);
  unlink (nfn);
}
END_TEST

START_TEST(file_make_backups)
{
  FILE      *fh;
  int       rc;
  char      buff [10];
  char      *r;

  char *ofn = "tmp/abc.txt";
  char *ofn0a = "tmp/abc.txt.0";
  char *ofn0 = "tmp/abc.txt.bak.0";
  char *ofn1 = "tmp/abc.txt.bak.1";
  char *ofn2 = "tmp/abc.txt.bak.2";
  char *ofn3 = "tmp/abc.txt.bak.3";
  unlink (ofn);
  unlink (ofn0a);
  unlink (ofn0);
  unlink (ofn1);
  unlink (ofn2);
  unlink (ofn3);

  fh = fopen (ofn, "w");
  ck_assert_ptr_nonnull (fh);
  fprintf (fh, "1\n");
  fclose (fh);

  rc = fileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn1);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn2);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn3);
  ck_assert_int_eq (rc, 0);

  makeBackups (ofn, 2);

  rc = fileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileExists (ofn2);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn3);
  ck_assert_int_eq (rc, 0);

  fh = fopen (ofn, "r");
  ck_assert_ptr_nonnull (fh);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "1");

  fh = fopen (ofn1, "r");
  ck_assert_ptr_nonnull (fh);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "1");

  fh = fopen (ofn, "w");
  ck_assert_ptr_nonnull (fh);
  fprintf (fh, "2\n");
  fclose (fh);

  makeBackups (ofn, 2);

  rc = fileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileExists (ofn2);
  ck_assert_int_ne (rc, 0);
  rc = fileExists (ofn3);
  ck_assert_int_eq (rc, 0);

  fh = fopen (ofn, "r");
  ck_assert_ptr_nonnull (fh);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "2");

  fh = fopen (ofn1, "r");
  ck_assert_ptr_nonnull (fh);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "2");

  fh = fopen (ofn2, "r");
  ck_assert_ptr_nonnull (fh);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "1");

  fh = fopen (ofn, "w");
  ck_assert_ptr_nonnull (fh);
  fprintf (fh, "3\n");
  fclose (fh);

  makeBackups (ofn, 2);

  rc = fileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileExists (ofn2);
  ck_assert_int_ne (rc, 0);
  rc = fileExists (ofn3);
  ck_assert_int_eq (rc, 0);

  fh = fopen (ofn, "r");
  ck_assert_ptr_nonnull (fh);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "3");

  fh = fopen (ofn1, "r");
  ck_assert_ptr_nonnull (fh);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "3");

  fh = fopen (ofn2, "r");
  ck_assert_ptr_nonnull (fh);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "2");

  unlink (ofn);
  unlink (ofn0a);
  unlink (ofn0);
  unlink (ofn1);
  unlink (ofn2);
}
END_TEST

START_TEST(file_delete)
{
  FILE      *fh;
  int       rc;

  char *fn = "tmp/def.txt";
  unlink (fn);
  fn = "tmp/abc.txt";
  fh = fopen (fn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = fileDelete (fn);
  ck_assert_int_eq (rc, 0);
  rc = fileDelete ("tmp/def.txt");
  ck_assert_int_lt (rc, 0);
  unlink (fn);
}
END_TEST

START_TEST(file_readall)
{
  FILE    *fh;
  char    *data;

  char *fn = "tmp/abc.txt";
  fh = fopen (fn, "w");
  fclose (fh);
  /* empty file */
  data = fileReadAll (fn);

  fh = fopen (fn, "w");
  fprintf (fh, "%s", "a");
  fclose (fh);
  /* one byte file */
  data = fileReadAll (fn);
  ck_assert_int_eq (strlen (data), 1);
  ck_assert_mem_eq (data, "a", 1);

  char *tdata = "lkjsdflkdjsfljsdfljsdfd\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tdata);
  fclose (fh);
  data = fileReadAll (fn);
  ck_assert_int_eq (strlen (data), strlen (tdata));
  ck_assert_mem_eq (data, tdata, strlen (tdata));
}
END_TEST

START_TEST(file_winpath)
{
  char    *from;
  char    to [MAXPATHLEN];

  from = "/tmp/abc.txt";
  fileConvWinPath (from, to, MAXPATHLEN);
  ck_assert_str_eq (to, "\\tmp\\abc.txt");
  ck_assert_int_eq (strlen (from), strlen (to));
}
END_TEST

Suite *
fileutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("File Utils Suite");
  tc = tcase_create ("File Utils");
  tcase_add_test (tc, fileinfo_chk);
  tcase_add_test (tc, file_exists);
  tcase_add_test (tc, file_copy);
  tcase_add_test (tc, file_move);
  tcase_add_test (tc, file_make_backups);
  tcase_add_test (tc, file_delete);
  tcase_add_test (tc, file_readall);
  tcase_add_test (tc, file_winpath);
  suite_add_tcase (s, tc);
  return s;
}

