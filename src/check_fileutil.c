#include "bdjconfig.h"

#include <stdio.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>

#include "fileutil.h"
#include "check_bdj.h"

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

  for (size_t i = 0; i < TCOUNT; ++i) {
    fi = fileInfo (tests[i].path);
    ck_assert_msg (fi->dlen == tests[i].dlen,
        "dlen: i:%ld %d/%d", i, fi->dlen, tests[i].dlen);
    ck_assert_msg (fi->blen == tests[i].blen,
        "blen: i:%ld %d/%d", i, fi->blen, tests[i].blen);
    ck_assert_msg (fi->flen == tests[i].flen,
        "flen: i:%ld %d/%d", i, fi->flen, tests[i].flen);
    ck_assert_msg (fi->elen == tests[i].elen,
        "elen: i:%ld %d/%d", i, fi->elen, tests[i].elen);
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
  ck_assert (fh != NULL);
  fclose (fh);
  rc = fileExists (fn);
  ck_assert (rc != 0);
  rc = fileExists ("tmp/def.txt");
  ck_assert (rc == 0);
  unlink (fn);
}
END_TEST

START_TEST(file_copy)
{
  FILE      *fh;
  int       rc;

  char *ofn = "tmp/abc.txt";
  char *nfn = "tmp/abc.txt.0";
  unlink (ofn);
  unlink (nfn);
  fh = fopen (ofn, "w");
  ck_assert (fh != NULL);
  fclose (fh);
  rc = fileCopy (ofn, nfn);
  ck_assert (rc == 0);
  rc = fileExists (ofn);
  ck_assert (rc != 0);
  rc = fileExists (nfn);
  ck_assert (rc != 0);
  unlink (ofn);
  unlink (nfn);
}
END_TEST

START_TEST(file_move)
{
  FILE      *fh;
  int       rc;

  char *ofn = "tmp/abc.txt";
  char *nfn = "tmp/abc.txt.0";
  unlink (ofn);
  unlink (nfn);
  fh = fopen (ofn, "w");
  ck_assert (fh != NULL);
  fclose (fh);
  rc = fileMove (ofn, nfn);
  ck_assert (rc == 0);
  rc = fileExists (ofn);
  ck_assert (rc == 0);
  rc = fileExists (nfn);
  ck_assert (rc != 0);
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
  char *ofn0 = "tmp/abc.txt.bak.0";
  char *ofn1 = "tmp/abc.txt.bak.1";
  char *ofn2 = "tmp/abc.txt.bak.2";
  char *ofn3 = "tmp/abc.txt.bak.3";
  unlink (ofn);
  unlink (ofn0);
  unlink (ofn1);
  unlink (ofn2);
  unlink (ofn3);

  fh = fopen (ofn, "w");
  ck_assert (fh != NULL);
  fprintf (fh, "1\n");
  fclose (fh);

  rc = fileExists (ofn);
  ck_assert (rc != 0);
  rc = fileExists (ofn0);
  ck_assert (rc == 0);
  rc = fileExists (ofn1);
  ck_assert (rc == 0);
  rc = fileExists (ofn2);
  ck_assert (rc == 0);
  rc = fileExists (ofn3);
  ck_assert (rc == 0);

  makeBackups (ofn, 2);

  rc = fileExists (ofn);
  ck_assert (rc != 0);
  rc = fileExists (ofn0);
  ck_assert (rc == 0);
  rc = fileExists (ofn1);
  ck_assert (rc != 0);
  rc = fileExists (ofn2);
  ck_assert (rc == 0);
  rc = fileExists (ofn3);
  ck_assert (rc == 0);

  fh = fopen (ofn, "r");
  ck_assert (fh != NULL);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert (strcmp (buff, "1") == 0);

  fh = fopen (ofn1, "r");
  ck_assert (fh != NULL);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert (strcmp (buff, "1") == 0);

  fh = fopen (ofn, "w");
  ck_assert (fh != NULL);
  fprintf (fh, "2\n");
  fclose (fh);

  makeBackups (ofn, 2);

  rc = fileExists (ofn);
  ck_assert (rc != 0);
  rc = fileExists (ofn0);
  ck_assert (rc == 0);
  rc = fileExists (ofn1);
  ck_assert (rc != 0);
  rc = fileExists (ofn2);
  ck_assert (rc != 0);
  rc = fileExists (ofn3);
  ck_assert (rc == 0);

  fh = fopen (ofn, "r");
  ck_assert (fh != NULL);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert (strcmp (buff, "2") == 0);

  fh = fopen (ofn1, "r");
  ck_assert (fh != NULL);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert (strcmp (buff, "2") == 0);

  fh = fopen (ofn2, "r");
  ck_assert (fh != NULL);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert (strcmp (buff, "1") == 0);

  fh = fopen (ofn, "w");
  ck_assert (fh != NULL);
  fprintf (fh, "3\n");
  fclose (fh);

  makeBackups (ofn, 2);

  rc = fileExists (ofn);
  ck_assert (rc != 0);
  rc = fileExists (ofn0);
  ck_assert (rc == 0);
  rc = fileExists (ofn1);
  ck_assert (rc != 0);
  rc = fileExists (ofn2);
  ck_assert (rc != 0);
  rc = fileExists (ofn3);
  ck_assert (rc == 0);

  fh = fopen (ofn, "r");
  ck_assert (fh != NULL);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert (strcmp (buff, "3") == 0);

  fh = fopen (ofn1, "r");
  ck_assert (fh != NULL);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert (strcmp (buff, "3") == 0);

  fh = fopen (ofn2, "r");
  ck_assert (fh != NULL);
  r = fgets (buff, 2, fh);
  fclose (fh);
  ck_assert (strcmp (buff, "2") == 0);

  unlink (ofn);
  unlink (ofn0);
  unlink (ofn1);
  unlink (ofn2);
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
  suite_add_tcase (s, tc);
  return s;
}

