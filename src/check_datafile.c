#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "fileop.h"
#include "datafile.h"
#include "log.h"
#include "check_bdj.h"

START_TEST(parse_init_free)
{
  parseinfo_t     *pi;

  logMsg (LOG_DBG, LOG_LVL_1, "=== parse_init_free");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  ck_assert_int_eq (pi->allocCount, 0);
  ck_assert_ptr_null (pi->strdata);
  parseFree (pi);
}
END_TEST

START_TEST(parse_simple)
{
  parseinfo_t     *pi;

  logMsg (LOG_DBG, LOG_LVL_1, "=== parse_simple");
  char *tstr = strdup ("# comment\nA\na\n# comment\nB\nb\nC\nc\nD\n# comment\nd\nE\ne\nF\nf");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  size_t count = parseSimple (pi, tstr);
  ck_assert_int_eq (count, 12);
  ck_assert_int_ge (pi->allocCount, 12);
  char **strdata = parseGetData (pi);
  ck_assert_ptr_eq (pi->strdata, strdata);
  ck_assert_str_eq (strdata[0], "A");
  ck_assert_str_eq (strdata[1], "a");
  ck_assert_str_eq (strdata[2], "B");
  ck_assert_str_eq (strdata[3], "b");
  ck_assert_str_eq (strdata[4], "C");
  ck_assert_str_eq (strdata[5], "c");
  ck_assert_str_eq (strdata[6], "D");
  ck_assert_str_eq (strdata[7], "d");
  ck_assert_str_eq (strdata[8], "E");
  ck_assert_str_eq (strdata[9], "e");
  ck_assert_str_eq (strdata[10], "F");
  ck_assert_str_eq (strdata[11], "f");
  parseFree (pi);
  free (tstr);
}
END_TEST

START_TEST(parse_keyvalue)
{
  parseinfo_t     *pi;

  logMsg (LOG_DBG, LOG_LVL_1, "=== parse_keyvalue");
  char *tstr = strdup ("A\n..a\nB\n..b\nC\n..c\nD\n..d\nE\n..e\nF\n..f");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  size_t count = parseKeyValue (pi, tstr);
  ck_assert_int_eq (count, 12);
  ck_assert_int_ge (pi->allocCount, 12);
  char **strdata = parseGetData (pi);
  ck_assert_ptr_eq (pi->strdata, strdata);
  ck_assert_str_eq (strdata[0], "A");
  ck_assert_str_eq (strdata[1], "a");
  ck_assert_str_eq (strdata[2], "B");
  ck_assert_str_eq (strdata[3], "b");
  ck_assert_str_eq (strdata[4], "C");
  ck_assert_str_eq (strdata[5], "c");
  ck_assert_str_eq (strdata[6], "D");
  ck_assert_str_eq (strdata[7], "d");
  ck_assert_str_eq (strdata[8], "E");
  ck_assert_str_eq (strdata[9], "e");
  ck_assert_str_eq (strdata[10], "F");
  ck_assert_str_eq (strdata[11], "f");
  parseFree (pi);
  free (tstr);
}
END_TEST

START_TEST(parse_with_comments)
{
  parseinfo_t     *pi;

  logMsg (LOG_DBG, LOG_LVL_1, "=== parse_with_comments");
  char *tstr = strdup ("# comment\nA\n..a\n# comment\nB\n..b\nC\n..c\nD\n# comment\n..d\nE\n..e\nF\n..f");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  size_t count = parseKeyValue (pi, tstr);
  ck_assert_int_eq (count, 12);
  ck_assert_int_ge (pi->allocCount, 12);
  char **strdata = parseGetData (pi);
  ck_assert_ptr_eq (pi->strdata, strdata);
  ck_assert_str_eq (strdata[0], "A");
  ck_assert_str_eq (strdata[1], "a");
  ck_assert_str_eq (strdata[2], "B");
  ck_assert_str_eq (strdata[3], "b");
  ck_assert_str_eq (strdata[4], "C");
  ck_assert_str_eq (strdata[5], "c");
  ck_assert_str_eq (strdata[6], "D");
  ck_assert_str_eq (strdata[7], "d");
  ck_assert_str_eq (strdata[8], "E");
  ck_assert_str_eq (strdata[9], "e");
  ck_assert_str_eq (strdata[10], "F");
  ck_assert_str_eq (strdata[11], "f");
  parseFree (pi);
  free (tstr);
}
END_TEST

START_TEST(datafile_simple)
{
  datafile_t *    df;
  char *          value;

  logMsg (LOG_DBG, LOG_LVL_1, "=== datafile_simple");
  char *fn = "tmp/dftesta.txt";
  char *tstr = strdup ("# comment\nA\na\n# comment\nB\nb\nC\nc\nD\n# comment\nd\nE\ne\nF\nf\n");
  FILE *fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk", DFTYPE_LIST, fn, NULL, 0, DATAFILE_NO_LOOKUP);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_LIST);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);

  list_t *list = datafileGetList (df);
  listStartIterator (list);
  value = listIterateData (list);
  ck_assert_str_eq (value, "A");
  value = listIterateData (list);
  ck_assert_str_eq (value, "a");
  value = listIterateData (list);
  ck_assert_str_eq (value, "B");
  value = listIterateData (list);
  ck_assert_str_eq (value, "b");
  value = listIterateData (list);
  ck_assert_str_eq (value, "C");
  value = listIterateData (list);
  ck_assert_str_eq (value, "c");
  value = listIterateData (list);
  ck_assert_str_eq (value, "D");
  value = listIterateData (list);
  ck_assert_str_eq (value, "d");
  value = listIterateData (list);
  ck_assert_str_eq (value, "E");
  value = listIterateData (list);
  ck_assert_str_eq (value, "e");
  value = listIterateData (list);
  ck_assert_str_eq (value, "F");
  value = listIterateData (list);
  ck_assert_str_eq (value, "f");
  value = listIterateData (list);
  ck_assert_ptr_null (value);
  value = listIterateData (list);
  ck_assert_str_eq (value, "A");

  datafileFree (df);
  free (tstr);
  unlink (fn);
}
END_TEST

START_TEST(datafile_keyval_nodfkey)
{
  datafile_t        *df;
  char *            key;
  char *            value;

  logMsg (LOG_DBG, LOG_LVL_1, "=== datafile_keyval_nodfkey");
  char *fn = "tmp/dftestb.txt";
  char *tstr = strdup ("A\n..a\nB\n..b\nC\n..c\nD\n..d\nE\n..e\nF\n..f\n");
  FILE *fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk", DFTYPE_KEY_VAL, fn, NULL, 0, DATAFILE_NO_LOOKUP);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_KEY_VAL);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);

  list_t *list = datafileGetList (df);
  slistStartIterator (list);
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "A");
  value = slistGetData (list, key);
  ck_assert_str_eq (value, "a");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "B");
  value = slistGetData (list, key);
  ck_assert_str_eq (value, "b");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "C");
  value = slistGetData (list, key);
  ck_assert_str_eq (value, "c");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "D");
  value = slistGetData (list, key);
  ck_assert_str_eq (value, "d");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "E");
  value = slistGetData (list, key);
  ck_assert_str_eq (value, "e");
  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "F");
  value = slistGetData (list, key);
  ck_assert_str_eq (value, "f");

  key = slistIterateKeyStr (list);
  ck_assert_ptr_null (key);

  key = slistIterateKeyStr (list);
  ck_assert_str_eq (key, "A");
  value = slistGetData (list, key);
  ck_assert_str_eq (value, "a");

  datafileFree (df);
  free (tstr);
  unlink (fn);
}
END_TEST

  static datafilekey_t dfkeyskl[] = {
    { "A", 14, VALUE_DATA, NULL },
    { "B", 15, VALUE_LONG, NULL },
    { "C", 16, VALUE_DATA, NULL },
    { "D", 17, VALUE_DATA, NULL },
    { "E", 18, VALUE_DATA, NULL },
    { "F", 19, VALUE_DATA, NULL },
  };

START_TEST(datafile_keyval_dfkey)
{
  datafile_t        *df;
  long              key;
  long              lval;
  char *            value;

  logMsg (LOG_DBG, LOG_LVL_1, "=== datafile_keyval_dfkey");
  char *fn = "tmp/dftestb.txt";
  char *tstr = strdup ("A\n..a\nB\n..5\nC\n..c\nD\n..d\nE\n..e\nF\n..f\n");
  FILE *fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk", DFTYPE_KEY_VAL, fn, dfkeyskl, 6, DATAFILE_NO_LOOKUP);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_KEY_VAL);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);

  list_t *list = datafileGetList (df);
  llistStartIterator (list);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 14);
  value = llistGetData (list, key);
  ck_assert_str_eq (value, "a");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 15);
  lval = llistGetLong (list, key);
  ck_assert_int_eq (lval, 5);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 16);
  value = llistGetData (list, key);
  ck_assert_str_eq (value, "c");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 17);
  value = llistGetData (list, key);
  ck_assert_str_eq (value, "d");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 18);
  value = llistGetData (list, key);
  ck_assert_str_eq (value, "e");
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 19);
  value = llistGetData (list, key);
  ck_assert_str_eq (value, "f");

  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, -1);

  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 14);
  value = llistGetData (list, key);
  ck_assert_str_eq (value, "a");

  datafileFree (df);
  free (tstr);
  unlink (fn);
}
END_TEST

START_TEST(datafile_keylong)
{
  datafile_t        *df;
  long              key;
  char *            value;
  list_t *          vallist;
  long              lval;

  logMsg (LOG_DBG, LOG_LVL_1, "=== datafile_keylong");
  char *fn = "tmp/dftestb.txt";
  char *tstr = strdup ("KEY\n..0\nA\n..a\nB\n..0\n"
      "KEY\n..1\nA\n..a\nB\n..1\n"
      "KEY\n..2\nA\n..a\nB\n..2\n"
      "KEY\n..3\nA\n..a\nB\n..3\n");
  FILE *fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk", DFTYPE_KEY_LONG, fn, dfkeyskl, 6, DATAFILE_NO_LOOKUP);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_KEY_LONG);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);

  list_t *list = datafileGetList (df);
  llistStartIterator (list);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 0L);

  vallist = llistGetData (list, key);
  llistStartIterator (vallist);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 14L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "a");
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 15L);
  lval = llistGetLong (vallist, key);
  ck_assert_int_eq (lval, 0L);

  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 1L);

  vallist = llistGetData (list, key);
  llistStartIterator (vallist);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 14L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "a");
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 15L);
  lval = llistGetLong (vallist, key);
  ck_assert_int_eq (lval, 1L);

  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 2L);

  vallist = llistGetData (list, key);
  llistStartIterator (vallist);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 14L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "a");
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 15L);
  lval = llistGetLong (vallist, key);
  ck_assert_int_eq (lval, 2L);

  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 3L);

  vallist = llistGetData (list, key);
  llistStartIterator (vallist);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 14L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "a");
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 15L);
  lval = llistGetLong (vallist, key);
  ck_assert_int_eq (lval, 3L);

  datafileFree (df);
  free (tstr);
  unlink (fn);
}
END_TEST

START_TEST(datafile_keylong_lookup)
{
  datafile_t        *df;
  long              key;
  char *            value;
  list_t *          vallist;
  long              lval;
  char              *keystr;

  logMsg (LOG_DBG, LOG_LVL_1, "=== datafile_keylong_lookup");
  char *fn = "tmp/dftestb.txt";
  char *tstr = strdup ("KEY\n..0\nA\n..a\nB\n..0\nC\n..4\n"
      "KEY\n..1\nA\n..b\nB\n..1\nC\n..5\n"
      "KEY\n..2\nA\n..c\nB\n..2\nC\n..6\n"
      "KEY\n..3\nA\n..d\nB\n..3\nC\n..7\n");
  FILE *fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk", DFTYPE_KEY_LONG, fn, dfkeyskl, 6, 14);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_KEY_LONG);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);
  ck_assert_ptr_nonnull (df->lookup);

  list_t *list = datafileGetList (df);
  llistStartIterator (list);
  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 0L);

  vallist = llistGetData (list, key);
  llistStartIterator (vallist);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 14L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "a");
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 15L);
  lval = llistGetLong (vallist, key);
  ck_assert_int_eq (lval, 0L);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 16L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "4");

  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 1L);

  vallist = llistGetData (list, key);
  llistStartIterator (vallist);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 14L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "b");
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 15L);
  lval = llistGetLong (vallist, key);
  ck_assert_int_eq (lval, 1L);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 16L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "5");

  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 2L);

  vallist = llistGetData (list, key);
  llistStartIterator (vallist);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 14L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "c");
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 15L);
  lval = llistGetLong (vallist, key);
  ck_assert_int_eq (lval, 2L);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 16L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "6");

  key = llistIterateKeyLong (list);
  ck_assert_int_eq (key, 3L);

  vallist = llistGetData (list, key);
  llistStartIterator (vallist);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 14L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "d");
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 15L);
  lval = llistGetLong (vallist, key);
  ck_assert_int_eq (lval, 3L);
  key = llistIterateKeyLong (vallist);
  ck_assert_int_eq (key, 16L);
  value = llistGetData (vallist, key);
  ck_assert_str_eq (value, "7");

  list_t *lookup = datafileGetLookup (df);
  ck_assert_ptr_nonnull (df->lookup);
  slistStartIterator (lookup);
  keystr = slistIterateKeyStr (lookup);
  ck_assert_str_eq (keystr, "a");
  lval = slistGetLong (lookup, keystr);
  ck_assert_int_eq (lval, 0L);
  keystr = slistIterateKeyStr (lookup);
  ck_assert_str_eq (keystr, "b");
  lval = slistGetLong (lookup, keystr);
  ck_assert_int_eq (lval, 1L);
  keystr = slistIterateKeyStr (lookup);
  ck_assert_str_eq (keystr, "c");
  lval = slistGetLong (lookup, keystr);
  ck_assert_int_eq (lval, 2L);
  keystr = slistIterateKeyStr (lookup);
  ck_assert_str_eq (keystr, "d");
  lval = slistGetLong (lookup, keystr);
  ck_assert_int_eq (lval, 3L);

  datafileFree (df);
  free (tstr);
  unlink (fn);
}
END_TEST

START_TEST(datafile_backup)
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

  rc = fileopExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn1);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn2);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn3);
  ck_assert_int_eq (rc, 0);

  datafileBackup (ofn, 2);

  rc = fileopExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileopExists (ofn2);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn3);
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

  datafileBackup (ofn, 2);

  rc = fileopExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileopExists (ofn2);
  ck_assert_int_ne (rc, 0);
  rc = fileopExists (ofn3);
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

  datafileBackup (ofn, 2);

  rc = fileopExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileopExists (ofn2);
  ck_assert_int_ne (rc, 0);
  rc = fileopExists (ofn3);
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

Suite *
datafile_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Datafile Suite");
  tc = tcase_create ("Datafile");
  tcase_add_test (tc, parse_init_free);
  tcase_add_test (tc, parse_simple);
  tcase_add_test (tc, parse_keyvalue);
  tcase_add_test (tc, parse_with_comments);
  tcase_add_test (tc, datafile_simple);
  tcase_add_test (tc, datafile_keyval_nodfkey);
  tcase_add_test (tc, datafile_keyval_dfkey);
  tcase_add_test (tc, datafile_keylong);
  tcase_add_test (tc, datafile_keylong_lookup);
  tcase_add_test (tc, datafile_backup);
  suite_add_tcase (s, tc);
  return s;
}

