#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "datafile.h"
#include "log.h"
#include "check_bdj.h"

START_TEST(parse_init_free)
{
  parseinfo_t     *pi;

  logMsg (LOG_DBG, "=== parse_init_free");
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

  logMsg (LOG_DBG, "=== parse_simple");
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

  logMsg (LOG_DBG, "=== parse_keyvalue");
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

  logMsg (LOG_DBG, "=== parse_with_comments");
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

  logMsg (LOG_DBG, "=== datafile_simple");
  char *fn = "tmp/dftesta.txt";
  char *tstr = strdup ("# comment\nA\na\n# comment\nB\nb\nC\nc\nD\n# comment\nd\nE\ne\nF\nf\n");
  FILE *fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAlloc (NULL, 0, fn, DFTYPE_LIST);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_LIST);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);

  list_t *list = datafileGetData (df);
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

  logMsg (LOG_DBG, "=== datafile_keyval_nodfkey");
  char *fn = "tmp/dftestb.txt";
  char *tstr = strdup ("A\n..a\nB\n..b\nC\n..c\nD\n..d\nE\n..e\nF\n..f\n");
  FILE *fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAlloc (NULL, 0, fn, DFTYPE_KEY_VAL);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_KEY_VAL);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);

  list_t *list = datafileGetData (df);
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

  logMsg (LOG_DBG, "=== datafile_keyval_dfkey");
  char *fn = "tmp/dftestb.txt";
  char *tstr = strdup ("A\n..a\nB\n..5\nC\n..c\nD\n..d\nE\n..e\nF\n..f\n");
  FILE *fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAlloc (dfkeyskl, 6, fn, DFTYPE_KEY_VAL);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_KEY_VAL);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);

  list_t *list = datafileGetData (df);
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

  logMsg (LOG_DBG, "=== datafile_keylong");
  char *fn = "tmp/dftestb.txt";
  char *tstr = strdup ("KEY\n..0\nA\n..a\nB\n..0\n"
      "KEY\n..1\nA\n..a\nB\n..1\n"
      "KEY\n..2\nA\n..a\nB\n..2\n"
      "KEY\n..3\nA\n..a\nB\n..3\n");
  FILE *fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAlloc (dfkeyskl, 6, fn, DFTYPE_KEY_LONG);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (df->dftype, DFTYPE_KEY_LONG);
  ck_assert_str_eq (df->fname, fn);
  ck_assert_ptr_nonnull (df->data);

  list_t *list = datafileGetData (df);
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
  suite_add_tcase (s, tc);
  return s;
}

