#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <check.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "fileop.h"
#include "datafile.h"
#include "log.h"
#include "nlist.h"
#include "check_bdj.h"

START_TEST(parse_init_free)
{
  parseinfo_t     *pi;

  logMsg (LOG_DBG, LOG_IMPORTANT, "=== parse_init_free");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  ck_assert_int_eq (pi->allocCount, 0);
  ck_assert_ptr_null (pi->strdata);
  parseFree (pi);
}
END_TEST

START_TEST(parse_simple)
{
  parseinfo_t     *pi = NULL;
  char            *tstr = NULL;
  ssize_t          count;
  char            **strdata = NULL;


  logMsg (LOG_DBG, LOG_IMPORTANT, "=== parse_simple");
  tstr = strdup ("# comment\nA\na\n# comment\nB\nb\nC\nc\nD\n# comment\nd\nE\ne\nF\nf");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  count = parseSimple (pi, tstr);
  ck_assert_int_eq (count, 12);
  ck_assert_int_ge (pi->allocCount, 12);
  strdata = parseGetData (pi);
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
  char            *tstr = NULL;
  ssize_t          count;
  char            **strdata = NULL;


  logMsg (LOG_DBG, LOG_IMPORTANT, "=== parse_keyvalue");
  tstr = strdup ("A\n..a\nB\n..b\nC\n..c\nD\n..d\nE\n..e\nF\n..f");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  count = parseKeyValue (pi, tstr);
  ck_assert_int_eq (count, 12);
  ck_assert_int_ge (pi->allocCount, 12);
  strdata = parseGetData (pi);
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
  char            *tstr = NULL;
  ssize_t          count;
  char            **strdata = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "=== parse_with_comments");
  tstr = strdup ("# comment\nA\n..a\n# comment\nB\n..b\nC\n..c\nD\n# comment\n..d\nE\n..e\nF\n..f");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  count = parseKeyValue (pi, tstr);
  ck_assert_int_eq (count, 12);
  ck_assert_int_ge (pi->allocCount, 12);
  strdata = parseGetData (pi);
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
  char            *tstr = NULL;
  char            *fn;
  FILE            *fh;
  list_t          *list;
  slistidx_t      iteridx;


  logMsg (LOG_DBG, LOG_IMPORTANT, "=== datafile_simple");
  fn = "tmp/dftesta.txt";
  tstr = strdup ("# comment\nA\na\n# comment\nB\nb\nC\nc\nD\n# comment\nd\nE\ne\nF\nf\n");
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-a", DFTYPE_LIST, fn, NULL, 0, DATAFILE_NO_LOOKUP);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_LIST);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);

  list = datafileGetList (df);
  slistStartIterator (list, &iteridx);
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "A");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "a");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "B");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "b");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "C");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "c");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "D");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "d");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "E");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "e");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "F");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "f");
  value = slistIterateKey (list, &iteridx);
  ck_assert_ptr_null (value);
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "A");

  datafileFree (df);
  free (tstr);
  unlink (fn);
}
END_TEST

  static datafilekey_t dfkeyskl[] = {
    { "A", 14, VALUE_DATA, NULL, -1 },
    { "B", 15, VALUE_NUM,  NULL, -1 },
    { "C", 16, VALUE_DATA,  NULL, -1 },
    { "D", 17, VALUE_NUM, NULL, -1 },
    { "E", 18, VALUE_DATA, NULL, -1 },
    { "F", 19, VALUE_DATA, NULL, -1 },
  };

START_TEST(datafile_keyval_dfkey)
{
  datafile_t        *df;
  listidx_t         key;
  ssize_t           lval;
  char *            value;
  char            *tstr = NULL;
  char            *fn = NULL;
  FILE            *fh;
  list_t          *list;
  nlistidx_t      iteridx;


  logMsg (LOG_DBG, LOG_IMPORTANT, "=== datafile_keyval_dfkey");
  fn = "tmp/dftestb.txt";
  tstr = strdup ("A\n..a\nB\n..5\nC\n..c\nD\n..6\nE\n..e\nF\n..f\n");
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-b", DFTYPE_KEY_VAL, fn, dfkeyskl, 6, DATAFILE_NO_LOOKUP);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_KEY_VAL);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);

  list = datafileGetList (df);
  nlistStartIterator (list, &iteridx);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 14);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "a");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 15);
  lval = nlistGetNum (list, key);
  ck_assert_int_eq (lval, 5);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 16);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "c");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 17);
  lval = nlistGetNum (list, key);
  ck_assert_int_eq (lval, 6);
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 18);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "e");
  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 19);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "f");

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 14);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "a");

  datafileFree (df);
  free (tstr);
  unlink (fn);
}
END_TEST

START_TEST(datafile_indirect)
{
  datafile_t      *df;
  listidx_t       key;
  char *          value;
  ssize_t         lval;
  char            *tstr = NULL;
  char            *fn = NULL;
  FILE            *fh;
  list_t          *list;
  ilistidx_t      iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "=== datafile_indirect");
  fn = "tmp/dftestb.txt";
  tstr = strdup ("KEY\n..0\nA\n..a\nB\n..0\n"
      "KEY\n..1\nA\n..a\nB\n..1\n"
      "KEY\n..2\nA\n..a\nB\n..2\n"
      "KEY\n..3\nA\n..a\nB\n..3\n");
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-c", DFTYPE_INDIRECT, fn, dfkeyskl, 6, DATAFILE_NO_LOOKUP);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_INDIRECT);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);

  list = datafileGetList (df);
  ilistStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 0);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = ilistGetStr (list, key, 14);
  ck_assert_str_eq (value, "a");

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 2);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 0);
  value = ilistGetStr (list, key, 14);
  ck_assert_str_eq (value, "a");

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 1);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = ilistGetStr (list, key, 14);
  ck_assert_str_eq (value, "a");

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 0);
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 0);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);

  datafileFree (df);
  free (tstr);
  unlink (fn);
}
END_TEST

START_TEST(datafile_indirect_lookup)
{
  datafile_t        *df;
  listidx_t         key;
  char            *value;
  ilist_t          *list;
  ssize_t           lval;
  char              *keystr;
  char            *tstr = NULL;
  char            *fn = NULL;
  FILE            *fh;
  slist_t          *lookup;
  ilistidx_t      iteridx;


  logMsg (LOG_DBG, LOG_IMPORTANT, "=== datafile_indirect_lookup");
  fn = "tmp/dftestb.txt";
  tstr = strdup ("KEY\n..0\nA\n..a\nB\n..0\nC\n..4\nD\n..4\n"
      "KEY\n..1\nA\n..b\nB\n..1\nC\n..5\nD\n..5\n"
      "KEY\n..2\nA\n..c\nB\n..2\nC\n..6\nD\n..6\n"
      "KEY\n..3\nA\n..d\nB\n..3\nC\n..7\nD\n..7\n");
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-d", DFTYPE_INDIRECT, fn, dfkeyskl, 6, 14);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_INDIRECT);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);
  ck_assert_ptr_nonnull (df->lookup);

  list = datafileGetList (df);

  ilistStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 0);
  value = ilistGetStr (list, key, 16);
  ck_assert_str_eq (value, "4");
  lval = ilistGetNum (list, key, 17);
  ck_assert_int_eq (lval, 4);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = ilistGetStr (list, key, 14);
  ck_assert_str_eq (value, "b");
  value = ilistGetStr (list, key, 16);
  ck_assert_str_eq (value, "5");
  lval = ilistGetNum (list, key, 17);
  ck_assert_int_eq (lval, 5);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 2);
  value = ilistGetStr (list, key, 16);
  ck_assert_str_eq (value, "6");
  lval = ilistGetNum (list, key, 17);
  ck_assert_int_eq (lval, 6);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 3);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 0);
  value = ilistGetStr (list, key, 14);
  ck_assert_str_eq (value, "a");

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 1);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = ilistGetStr (list, key, 14);
  ck_assert_str_eq (value, "c");
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 2);
  value = ilistGetStr (list, key, 16);
  ck_assert_str_eq (value, "6");
  lval = ilistGetNum (list, key, 17);
  ck_assert_int_eq (lval, 6);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 0);
  value = ilistGetStr (list, key, 14);
  ck_assert_str_eq (value, "a");

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 1);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = ilistGetStr (list, key, 16);
  ck_assert_str_eq (value, "6");
  lval = ilistGetNum (list, key, 17);
  ck_assert_int_eq (lval, 6);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 0);
  value = ilistGetStr (list, key, 14);
  ck_assert_str_eq (value, "a");

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 1);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  value = ilistGetStr (list, key, 16);
  ck_assert_str_eq (value, "6");
  lval = ilistGetNum (list, key, 17);
  ck_assert_int_eq (lval, 6);

  lookup = datafileGetLookup (df);
  ck_assert_ptr_nonnull (df->lookup);
  listStartIterator (lookup, &iteridx);

  keystr = slistIterateKey (lookup, &iteridx);
  ck_assert_str_eq (keystr, "a");
  lval = listGetNum (lookup, keystr);
  ck_assert_int_eq (lval, 0);

  keystr = slistIterateKey (lookup, &iteridx);
  ck_assert_str_eq (keystr, "b");
  lval = listGetNum (lookup, keystr);
  ck_assert_int_eq (lval, 1);

  keystr = slistIterateKey (lookup, &iteridx);
  ck_assert_str_eq (keystr, "c");
  lval = listGetNum (lookup, keystr);
  ck_assert_int_eq (lval, 2);

  keystr = slistIterateKey (lookup, &iteridx);
  ck_assert_str_eq (keystr, "d");
  lval = listGetNum (lookup, keystr);
  ck_assert_int_eq (lval, 3);

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

  rc = fileopFileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn1);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn2);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn3);
  ck_assert_int_eq (rc, 0);

  datafileBackup (ofn, 2);

  rc = fileopFileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn2);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn3);
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

  rc = fileopFileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn2);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn3);
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

  rc = fileopFileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn2);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn3);
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
  tcase_add_test (tc, datafile_keyval_dfkey);
  tcase_add_test (tc, datafile_indirect);
  tcase_add_test (tc, datafile_indirect_lookup);
  tcase_add_test (tc, datafile_backup);
  suite_add_tcase (s, tc);
  return s;
}

