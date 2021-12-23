#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "datafile.h"
#include "check_bdj.h"

START_TEST(parse_init_free)
{
  parseinfo_t     *pi;

  pi = parseInit ();
  ck_assert (pi != NULL);
  ck_assert (pi->allocCount == 0);
  ck_assert (pi->strdata == NULL);
  parseFree (pi);
}
END_TEST

START_TEST(parse_basic)
{
  parseinfo_t     *pi;

  char *tstr = strdup ("A\n..a\nB\n..b\nC\n..c\nD\n..d\nE\n..e\nF\n..f");
  pi = parseInit ();
  ck_assert (pi != NULL);
  size_t count = parseKeyValue (pi, tstr);
  ck_assert (count == 12);
  ck_assert (pi->allocCount >= 12);
  char **strdata = parseGetData (pi);
  ck_assert (pi->strdata == strdata);
  ck_assert (strcmp (strdata[0], "A") == 0);
  ck_assert (strcmp (strdata[1], "a") == 0);
  ck_assert (strcmp (strdata[2], "B") == 0);
  ck_assert (strcmp (strdata[3], "b") == 0);
  ck_assert (strcmp (strdata[4], "C") == 0);
  ck_assert (strcmp (strdata[5], "c") == 0);
  ck_assert (strcmp (strdata[6], "D") == 0);
  ck_assert (strcmp (strdata[7], "d") == 0);
  ck_assert (strcmp (strdata[8], "E") == 0);
  ck_assert (strcmp (strdata[9], "e") == 0);
  ck_assert (strcmp (strdata[10], "F") == 0);
  ck_assert (strcmp (strdata[11], "f") == 0);
  parseFree (pi);
  free (tstr);
}
END_TEST

START_TEST(parse_with_comments)
{
  parseinfo_t     *pi;

  char *tstr = strdup ("# comment\nA\n..a\n# comment\nB\n..b\nC\n..c\nD\n# comment\n..d\nE\n..e\nF\n..f");
  pi = parseInit ();
  ck_assert (pi != NULL);
  size_t count = parseKeyValue (pi, tstr);
  ck_assert (count == 12);
  ck_assert (pi->allocCount >= 12);
  char **strdata = parseGetData (pi);
  ck_assert (pi->strdata == strdata);
  ck_assert (strcmp (strdata[0], "A") == 0);
  ck_assert (strcmp (strdata[1], "a") == 0);
  ck_assert (strcmp (strdata[2], "B") == 0);
  ck_assert (strcmp (strdata[3], "b") == 0);
  ck_assert (strcmp (strdata[4], "C") == 0);
  ck_assert (strcmp (strdata[5], "c") == 0);
  ck_assert (strcmp (strdata[6], "D") == 0);
  ck_assert (strcmp (strdata[7], "d") == 0);
  ck_assert (strcmp (strdata[8], "E") == 0);
  ck_assert (strcmp (strdata[9], "e") == 0);
  ck_assert (strcmp (strdata[10], "F") == 0);
  ck_assert (strcmp (strdata[11], "f") == 0);
  parseFree (pi);
  free (tstr);
}
END_TEST

START_TEST(parse_simple)
{
  parseinfo_t     *pi;

  char *tstr = strdup ("# comment\nA\na\n# comment\nB\nb\nC\nc\nD\n# comment\nd\nE\ne\nF\nf");
  pi = parseInit ();
  ck_assert (pi != NULL);
  size_t count = parseSimple (pi, tstr);
  ck_assert (count == 12);
  ck_assert (pi->allocCount >= 12);
  char **strdata = parseGetData (pi);
  ck_assert (pi->strdata == strdata);
  ck_assert (strcmp (strdata[0], "A") == 0);
  ck_assert (strcmp (strdata[1], "a") == 0);
  ck_assert (strcmp (strdata[2], "B") == 0);
  ck_assert (strcmp (strdata[3], "b") == 0);
  ck_assert (strcmp (strdata[4], "C") == 0);
  ck_assert (strcmp (strdata[5], "c") == 0);
  ck_assert (strcmp (strdata[6], "D") == 0);
  ck_assert (strcmp (strdata[7], "d") == 0);
  ck_assert (strcmp (strdata[8], "E") == 0);
  ck_assert (strcmp (strdata[9], "e") == 0);
  ck_assert (strcmp (strdata[10], "F") == 0);
  ck_assert (strcmp (strdata[11], "f") == 0);
  parseFree (pi);
  free (tstr);
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
parse_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Datafile Suite");
  tc = tcase_create ("Datafile");
  tcase_add_test (tc, parse_init_free);
  tcase_add_test (tc, parse_basic);
  tcase_add_test (tc, parse_with_comments);
  tcase_add_test (tc, parse_simple);
  tcase_add_test (tc, file_exists);
  tcase_add_test (tc, file_copy);
  tcase_add_test (tc, file_move);
  tcase_add_test (tc, file_make_backups);
  suite_add_tcase (s, tc);
  return s;
}

