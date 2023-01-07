/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "fileop.h"
#include "datafile.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "check_bdj.h"

START_TEST(parse_init_free)
{
  parseinfo_t     *pi;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- parse_init_free");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  ck_assert_int_eq (parseGetAllocCount (pi), 0);
  ck_assert_ptr_null (parseGetData (pi));
  parseFree (pi);
}
END_TEST

START_TEST(parse_simple)
{
  parseinfo_t     *pi = NULL;
  char            *tstr = NULL;
  ssize_t          count;
  char            **strdata = NULL;
  int             vers;


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- parse_simple");
  tstr = mdstrdup ("# comment\n# version 2\nA\na\n# comment\nB\nb\nC\nc\nD\n# comment\nd\nE\ne\nF\nf\nG\n1200\n");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  count = parseSimple (pi, tstr, &vers);
  ck_assert_int_eq (vers, 2);
  ck_assert_int_eq (count, 14);
  ck_assert_int_ge (parseGetAllocCount (pi), 14);
  strdata = parseGetData (pi);
  ck_assert_ptr_eq (parseGetData (pi), strdata);
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
  ck_assert_str_eq (strdata[12], "G");
  ck_assert_str_eq (strdata[13], "1200");
  parseFree (pi);
  mdfree (tstr);
}
END_TEST

START_TEST(parse_keyvalue)
{
  parseinfo_t     *pi;
  char            *tstr = NULL;
  ssize_t          count;
  char            **strdata = NULL;


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- parse_keyvalue");
  tstr = mdstrdup ("version\n..3\nA\n..a\nB\n..b\nC\n..c\nD\n..d\nE\n..e\nF\n..f\nG\n..1200\n");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  count = parseKeyValue (pi, tstr);
  ck_assert_int_eq (count, 16);
  ck_assert_int_ge (parseGetAllocCount (pi), 16);
  strdata = parseGetData (pi);
  ck_assert_ptr_eq (parseGetData (pi), strdata);
  ck_assert_str_eq (strdata[0], "version");
  ck_assert_str_eq (strdata[1], "3");
  ck_assert_str_eq (strdata[2], "A");
  ck_assert_str_eq (strdata[3], "a");
  ck_assert_str_eq (strdata[4], "B");
  ck_assert_str_eq (strdata[5], "b");
  ck_assert_str_eq (strdata[6], "C");
  ck_assert_str_eq (strdata[7], "c");
  ck_assert_str_eq (strdata[8], "D");
  ck_assert_str_eq (strdata[9], "d");
  ck_assert_str_eq (strdata[10], "E");
  ck_assert_str_eq (strdata[11], "e");
  ck_assert_str_eq (strdata[12], "F");
  ck_assert_str_eq (strdata[13], "f");
  ck_assert_str_eq (strdata[14], "G");
  ck_assert_str_eq (strdata[15], "1200");
  parseFree (pi);
  mdfree (tstr);
}
END_TEST

START_TEST(parse_with_comments)
{
  parseinfo_t     *pi;
  char            *tstr = NULL;
  ssize_t          count;
  char            **strdata = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- parse_with_comments");
  tstr = mdstrdup ("# comment\nversion\n..4\nA\n..a\n# comment\nB\n..b\nC\n..c\nD\n# comment\n..d\nE\n..e\nF\n..f\nG\n..1200\n");
  pi = parseInit ();
  ck_assert_ptr_nonnull (pi);
  count = parseKeyValue (pi, tstr);
  ck_assert_int_eq (count, 16);
  ck_assert_int_ge (parseGetAllocCount (pi), 16);
  strdata = parseGetData (pi);
  ck_assert_ptr_eq (parseGetData (pi), strdata);
  ck_assert_str_eq (strdata[0], "version");
  ck_assert_str_eq (strdata[1], "4");
  ck_assert_str_eq (strdata[2], "A");
  ck_assert_str_eq (strdata[3], "a");
  ck_assert_str_eq (strdata[4], "B");
  ck_assert_str_eq (strdata[5], "b");
  ck_assert_str_eq (strdata[6], "C");
  ck_assert_str_eq (strdata[7], "c");
  ck_assert_str_eq (strdata[8], "D");
  ck_assert_str_eq (strdata[9], "d");
  ck_assert_str_eq (strdata[10], "E");
  ck_assert_str_eq (strdata[11], "e");
  ck_assert_str_eq (strdata[12], "F");
  ck_assert_str_eq (strdata[13], "f");
  ck_assert_str_eq (strdata[14], "G");
  ck_assert_str_eq (strdata[15], "1200");
  parseFree (pi);
  mdfree (tstr);
}
END_TEST

START_TEST(datafile_simple)
{
  datafile_t *    df;
  char *          value;
  char            *tstr = NULL;
  char            *fn;
  FILE            *fh;
  slist_t         *list;
  slistidx_t      iteridx;
  int             vers;


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- datafile_simple");
  fn = "tmp/dftesta.txt";
  tstr = "# comment\n# version 5\nA\na\n# comment\nB\nb\nC\nc\nD\n# comment\nd\nE\ne\nF\nf\nG\n1200\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-a", DFTYPE_LIST, fn, NULL, 0);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (datafileGetType (df), DFTYPE_LIST);
  ck_assert_str_eq (datafileGetFname (df), fn);
  ck_assert_ptr_nonnull (datafileGetData (df));

  list = datafileGetList (df);
  slistStartIterator (list, &iteridx);
  vers = slistGetVersion (list);
  ck_assert_int_eq (vers, 5);
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
  ck_assert_str_eq (value, "G");
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "1200");
  value = slistIterateKey (list, &iteridx);
  ck_assert_ptr_null (value);
  value = slistIterateKey (list, &iteridx);
  ck_assert_str_eq (value, "A");

  datafileFree (df);
  unlink (fn);
}
END_TEST

static datafilekey_t dfkeyskl[] = {
  { "A", 14, VALUE_STR, NULL, -1 },
  { "B", 15, VALUE_NUM,  NULL, -1 },
  { "C", 16, VALUE_STR,  NULL, -1 },
  { "D", 17, VALUE_NUM, convBoolean, -1 },
  { "E", 18, VALUE_STR, NULL, -1 },
  { "F", 19, VALUE_STR, NULL, -1 },
  { "G", 20, VALUE_DOUBLE, NULL, -1 },
  { "H", 21, VALUE_LIST, convTextList, -1 },
  { "I", 22, VALUE_NUM, convBoolean, -1 },
  { "J", 23, VALUE_NUM, convBoolean, -1 },
  { "K", 24, VALUE_NUM, convBoolean, -1 },
};
#define DFKEY_COUNT 11

START_TEST(datafile_keyval_dfkey)
{
  datafile_t        *df;
  listidx_t         key;
  ssize_t           lval;
  double            dval;
  char *            value;
  char            *tstr = NULL;
  char            *fn = NULL;
  FILE            *fh;
  slist_t         *list;
  slist_t         *slist;
  nlistidx_t      iteridx;
  slistidx_t      siteridx;
  int             vers;


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- datafile_keyval_dfkey");
  fn = "tmp/dftestb.txt";
  tstr = "version\n..6\nA\n..a\nB\n..5\nC\n..c\nD\n..on\nE\n..e\nF\n..f\nG\n..1200\nH\n..aaa bbb ccc\nI\n..off\nJ\n..yes\nK\n..no\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-b", DFTYPE_KEY_VAL, fn, dfkeyskl, DFKEY_COUNT);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (datafileGetType (df), DFTYPE_KEY_VAL);
  ck_assert_str_eq (datafileGetFname (df), fn);
  ck_assert_ptr_nonnull (datafileGetData (df));

  list = datafileGetList (df);
  vers = nlistGetVersion (list);
  ck_assert_int_eq (vers, 6);

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
  ck_assert_int_eq (lval, 1);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 18);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "e");

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 19);
  value = nlistGetData (list, key);
  ck_assert_str_eq (value, "f");

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 20);
  dval = nlistGetDouble (list, key);
  ck_assert_float_eq (dval, 1.2);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 21);
  slist = nlistGetList (list, key);

  slistStartIterator (slist, &siteridx);
  value = slistIterateKey (slist, &siteridx);
  ck_assert_str_eq (value, "aaa");
  value = slistIterateKey (slist, &siteridx);
  ck_assert_str_eq (value, "bbb");
  value = slistIterateKey (slist, &siteridx);
  ck_assert_str_eq (value, "ccc");

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 22);
  lval = nlistGetNum (list, key);
  ck_assert_int_eq (lval, 0);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 23);
  lval = nlistGetNum (list, key);
  ck_assert_int_eq (lval, 1);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 24);
  lval = nlistGetNum (list, key);
  ck_assert_int_eq (lval, 0);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 14);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "a");

  datafileFree (df);
  unlink (fn);
}
END_TEST

START_TEST(datafile_keyval_df_extra)
{
  datafile_t        *df;
  listidx_t         key;
  ssize_t           lval;
  double            dval;
  char *            value;
  char            *tstr = NULL;
  char            *fn = NULL;
  FILE            *fh;
  slist_t         *list;
  slist_t         *slist;
  nlistidx_t      iteridx;
  slistidx_t      siteridx;
  int             vers;


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- datafile_keyval_df_extra");
  fn = "tmp/dftestc.txt";
  tstr = "version\n..7\nA\n..a\nB\n..5\nQQ\n..qq\nC\n..c\nD\n..on\nE\n..e\nF\n..f\nG\n..1200\nH\n..aaa bbb ccc\nI\n..off\nJ\n..yes\nK\n..no\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-c", DFTYPE_KEY_VAL, fn, dfkeyskl, DFKEY_COUNT);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (datafileGetType (df), DFTYPE_KEY_VAL);
  ck_assert_str_eq (datafileGetFname (df), fn);
  ck_assert_ptr_nonnull (datafileGetData (df));

  list = datafileGetList (df);

  vers = nlistGetVersion (list);
  ck_assert_int_eq (vers, 7);

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
  ck_assert_int_eq (lval, 1);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 18);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "e");

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 19);
  value = nlistGetData (list, key);
  ck_assert_str_eq (value, "f");

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 20);
  dval = nlistGetDouble (list, key);
  ck_assert_float_eq (dval, 1.2);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 21);
  slist = nlistGetList (list, key);

  slistStartIterator (slist, &siteridx);
  value = slistIterateKey (slist, &siteridx);
  ck_assert_str_eq (value, "aaa");
  value = slistIterateKey (slist, &siteridx);
  ck_assert_str_eq (value, "bbb");
  value = slistIterateKey (slist, &siteridx);
  ck_assert_str_eq (value, "ccc");

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 22);
  lval = nlistGetNum (list, key);
  ck_assert_int_eq (lval, 0);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 23);
  lval = nlistGetNum (list, key);
  ck_assert_int_eq (lval, 1);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 24);
  lval = nlistGetNum (list, key);
  ck_assert_int_eq (lval, 0);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);

  key = nlistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 14);
  value = nlistGetStr (list, key);
  ck_assert_str_eq (value, "a");

  datafileFree (df);
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
  ilist_t         *list;
  ilistidx_t      iteridx;
  int             vers;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- datafile_indirect");
  fn = "tmp/dftestd.txt";
  tstr = "version\n..8\nKEY\n..0\nA\n..a\nB\n..0\n"
      "KEY\n..1\nA\n..a\nB\n..1\n"
      "KEY\n..2\nA\n..a\nB\n..2\n"
      "KEY\n..3\nA\n..a\nB\n..3\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-d", DFTYPE_INDIRECT, fn, dfkeyskl, DFKEY_COUNT);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (datafileGetType (df), DFTYPE_INDIRECT);
  ck_assert_str_eq (datafileGetFname (df), fn);
  ck_assert_ptr_nonnull (datafileGetData (df));

  list = datafileGetList (df);

  vers = ilistGetVersion (list);
  ck_assert_int_eq (vers, 8);

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
  unlink (fn);
}
END_TEST

START_TEST(datafile_indirect_missing)
{
  datafile_t      *df;
  listidx_t       key;
  char *          value;
  ssize_t         lval;
  char            *tstr = NULL;
  char            *fn = NULL;
  FILE            *fh;
  ilist_t         *list;
  ilistidx_t      iteridx;
  int             vers;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- datafile_indirect_missing");
  fn = "tmp/dfteste.txt";
  tstr = "version\n..10\nKEY\n..0\nA\n..a\nB\n..0\n"
      "KEY\n..1\nA\n..a\n"
      "KEY\n..2\nA\n..a\nB\n..2\n"
      "KEY\n..3\nB\n..3\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-f", DFTYPE_INDIRECT, fn, dfkeyskl, DFKEY_COUNT);
  ck_assert_ptr_nonnull (df);
  ck_assert_int_eq (datafileGetType (df), DFTYPE_INDIRECT);
  ck_assert_str_eq (datafileGetFname (df), fn);
  ck_assert_ptr_nonnull (datafileGetData (df));

  list = datafileGetList (df);

  vers = ilistGetVersion (list);
  ck_assert_int_eq (vers, 10);

  ilistStartIterator (list, &iteridx);
  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 0);
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 0);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);
  value = ilistGetStr (list, key, 14);
  ck_assert_str_eq (value, "a");
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, LIST_VALUE_INVALID);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 2);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = ilistGetStr (list, key, 14);
  ck_assert_ptr_null (value);
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
  ck_assert_int_eq (lval, LIST_VALUE_INVALID);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 2);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 3);
  value = ilistGetStr (list, key, 14);
  ck_assert_ptr_null (value);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, -1);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 0);
  lval = ilistGetNum (list, key, 15);
  ck_assert_int_eq (lval, 0);

  key = ilistIterateKey (list, &iteridx);
  ck_assert_int_eq (key, 1);

  datafileFree (df);
  unlink (fn);
}
END_TEST


START_TEST(datafile_keyval_savelist)
{
  datafile_t      *df;
  char *          value;
  char            *tstr = NULL;
  char            *fn = NULL;
  FILE            *fh;
  nlist_t         *list;
  slist_t         *slist;
  int             vers;
  char            tmp [40];


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- datafile_keyval_savelist");
  fn = "tmp/dftestf.txt";
  tstr = "version\n..11\nA\n..a\nB\n..5\nC\n..c\nD\n..on\nE\n..e\nF\n..f\nG\n..1200\nH\n..aaa bbb ccc\nI\n..off\nJ\n..yes\nK\n..no\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-g", DFTYPE_KEY_VAL, fn, dfkeyskl, DFKEY_COUNT);

  list = datafileGetList (df);
  vers = nlistGetVersion (list);
  ck_assert_int_eq (vers, 11);

  slist = datafileSaveKeyValList ("chk-df-g", dfkeyskl, DFKEY_COUNT, list);

  for (int i = 0; i < DFKEY_COUNT; ++i) {
    tstr = slistGetStr (slist, dfkeyskl [i].name);
    if (dfkeyskl [i].valuetype == VALUE_STR) {
      value = nlistGetStr (list, dfkeyskl [i].itemkey);
      ck_assert_str_eq (value, tstr);
    }
    if (dfkeyskl [i].valuetype == VALUE_DOUBLE) {
      snprintf (tmp, sizeof (tmp), "%.0f",
          nlistGetDouble (list, dfkeyskl [i].itemkey) * DF_DOUBLE_MULT);
      ck_assert_str_eq (tmp, tstr);
    }
    if (dfkeyskl [i].valuetype == VALUE_NUM) {
      if (dfkeyskl [i].convFunc != NULL) {
        datafileconv_t  conv;

        conv.allocated = false;
        conv.valuetype = VALUE_NUM;
        conv.num = nlistGetNum (list, dfkeyskl [i].itemkey);
        dfkeyskl [i].convFunc (&conv);
        value = conv.str;
        ck_assert_str_eq (value, tstr);
        if (conv.allocated) {
          mdfree (conv.str);
        }
      } else {
        snprintf (tmp, sizeof (tmp), "%"PRId64,
            nlistGetNum (list, dfkeyskl [i].itemkey));
        ck_assert_str_eq (tmp, tstr);
      }
    }
    if (dfkeyskl [i].valuetype == VALUE_LIST) {
      if (dfkeyskl [i].convFunc != NULL) {
        datafileconv_t  conv;

        conv.allocated = false;
        conv.valuetype = VALUE_LIST;
        conv.list = nlistGetList (list, dfkeyskl [i].itemkey);
        dfkeyskl [i].convFunc (&conv);
        value = conv.str;
        ck_assert_str_eq (value, tstr);
        if (conv.allocated) {
          mdfree (conv.str);
        }
      }
    }
  }

  datafileFree (df);
  slistFree (slist);
  unlink (fn);
}
END_TEST

START_TEST(datafile_keyval_savebuffer)
{
  datafile_t      *df;
  datafile_t      *tdf;
  char            *fn = NULL;
  FILE            *fh;
  char            *tstr;
  nlist_t         *list;
  nlist_t         *tlist;
  int             vers;
  char            tbuff [4096];


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- datafile_keyval_savebuffer");
  fn = "tmp/dftestg.txt";
  tstr = "version\n..12\nA\n..a\nB\n..5\nC\n..c\nD\n..on\nE\n..e\nF\n..f\nG\n..1200\nH\n..aaa bbb ccc\nI\n..off\nJ\n..yes\nK\n..no\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-h", DFTYPE_KEY_VAL, fn, dfkeyskl, DFKEY_COUNT);
  unlink (fn);

  list = datafileGetList (df);
  vers = nlistGetVersion (list);
  ck_assert_int_eq (vers, 12);

  datafileSaveKeyValBuffer (tbuff, sizeof (tbuff), "chk-df-h", dfkeyskl, DFKEY_COUNT, list, 0);
  fn = "tmp/dftesth.txt";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tbuff);
  fclose (fh);
  tdf = datafileAllocParse ("chk-df-h", DFTYPE_KEY_VAL, fn, dfkeyskl, DFKEY_COUNT);
  tlist = datafileGetList (tdf);

  for (int i = 0; i < DFKEY_COUNT; ++i) {
    if (dfkeyskl [i].valuetype == VALUE_STR) {
      ck_assert_str_eq (nlistGetStr (list, dfkeyskl [i].itemkey),
          nlistGetStr (tlist, dfkeyskl [i].itemkey));
    }
    if (dfkeyskl [i].valuetype == VALUE_DOUBLE) {
      ck_assert_float_eq (nlistGetDouble (list, dfkeyskl [i].itemkey),
          nlistGetDouble (tlist, dfkeyskl [i].itemkey));
    }
    if (dfkeyskl [i].valuetype == VALUE_NUM) {
      ck_assert_int_eq (nlistGetNum (list, dfkeyskl [i].itemkey),
          nlistGetNum (tlist, dfkeyskl [i].itemkey));
    }
    if (dfkeyskl [i].valuetype == VALUE_LIST) {
      if (dfkeyskl [i].convFunc != NULL) {
        datafileconv_t  conv;
        datafileconv_t  tconv;

        conv.allocated = false;
        conv.valuetype = VALUE_LIST;
        conv.list = nlistGetList (list, dfkeyskl [i].itemkey);
        dfkeyskl [i].convFunc (&conv);

        tconv.allocated = false;
        tconv.valuetype = VALUE_LIST;
        tconv.list = nlistGetList (list, dfkeyskl [i].itemkey);
        dfkeyskl [i].convFunc (&tconv);
        ck_assert_str_eq (conv.str, tconv.str);

        if (conv.allocated) {
          mdfree (conv.str);
        }
        if (tconv.allocated) {
          mdfree (tconv.str);
        }
      }
    }
  }

  datafileFree (df);
  datafileFree (tdf);
  unlink (fn);
}
END_TEST

START_TEST(datafile_keyval_save)
{
  datafile_t      *df;
  datafile_t      *tdf;
  char            *fn = NULL;
  FILE            *fh;
  char            *tstr;
  nlist_t         *list;
  nlist_t         *tlist;
  int             vers;


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- datafile_keyval_save");
  fn = "tmp/dftestk.txt";
  tstr = "version\n..13\nA\n..a\nB\n..5\nC\n..c\nD\n..on\nE\n..e\nF\n..f\nG\n..1200\nH\n..aaa bbb ccc\nI\n..off\nJ\n..yes\nK\n..no\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-i", DFTYPE_KEY_VAL, fn, dfkeyskl, DFKEY_COUNT);
  unlink (fn);

  list = datafileGetList (df);
  vers = nlistGetVersion (list);
  ck_assert_int_eq (vers, 13);

  fn = "tmp/dftestl.txt";
  datafileSaveKeyVal ("chk-df-i", fn, dfkeyskl, DFKEY_COUNT, list, 0);
  tdf = datafileAllocParse ("chk-df-i", DFTYPE_KEY_VAL, fn, dfkeyskl, DFKEY_COUNT);
  tlist = datafileGetList (tdf);
  vers = nlistGetVersion (tlist);
  ck_assert_int_eq (vers, 13);

  for (int i = 0; i < DFKEY_COUNT; ++i) {
    if (dfkeyskl [i].valuetype == VALUE_STR) {
      ck_assert_str_eq (nlistGetStr (list, dfkeyskl [i].itemkey),
          nlistGetStr (tlist, dfkeyskl [i].itemkey));
    }
    if (dfkeyskl [i].valuetype == VALUE_DOUBLE) {
      ck_assert_float_eq (nlistGetDouble (list, dfkeyskl [i].itemkey),
          nlistGetDouble (tlist, dfkeyskl [i].itemkey));
    }
    if (dfkeyskl [i].valuetype == VALUE_NUM) {
      ck_assert_int_eq (nlistGetNum (list, dfkeyskl [i].itemkey),
          nlistGetNum (tlist, dfkeyskl [i].itemkey));
    }
    if (dfkeyskl [i].valuetype == VALUE_LIST) {
      if (dfkeyskl [i].convFunc != NULL) {
        datafileconv_t  conv;
        datafileconv_t  tconv;

        conv.allocated = false;
        conv.valuetype = VALUE_LIST;
        conv.list = nlistGetList (list, dfkeyskl [i].itemkey);
        dfkeyskl [i].convFunc (&conv);

        tconv.allocated = false;
        tconv.valuetype = VALUE_LIST;
        tconv.list = nlistGetList (list, dfkeyskl [i].itemkey);
        dfkeyskl [i].convFunc (&tconv);
        ck_assert_str_eq (conv.str, tconv.str);

        if (conv.allocated) {
          mdfree (conv.str);
        }
        if (tconv.allocated) {
          mdfree (tconv.str);
        }
      }
    }
  }

  datafileFree (df);
  datafileFree (tdf);
  unlink (fn);
}
END_TEST

START_TEST(datafile_indirect_save)
{
  datafile_t      *df;
  datafile_t      *tdf;
  char            *tstr = NULL;
  char            *fn = NULL;
  FILE            *fh;
  ilist_t         *list;
  ilist_t         *tlist;
  int             vers;
  ilistidx_t      key;
  ilistidx_t      iteridx;
  int             nval;
  int             tnval;
  char            *sval;
  char            *tsval;


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- datafile_indirect_save");
  fn = "tmp/dftestm.txt";
  tstr = "version\n..14\nKEY\n..0\nA\n..a\nB\n..0\n"
      "KEY\n..1\nA\n..b\nB\n..2\n"
      "KEY\n..2\nA\n..c\nB\n..3\n"
      "KEY\n..3\nA\n..d\nB\n..4\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-j", DFTYPE_INDIRECT, fn, dfkeyskl, DFKEY_COUNT);
  list = datafileGetList (df);
  vers = ilistGetVersion (list);
  ck_assert_int_eq (vers, 14);
  unlink (fn);

  fn = "tmp/dftestn.txt";
  datafileSaveIndirect ("chk-df-j", fn, dfkeyskl, DFKEY_COUNT, list);
  tdf = datafileAllocParse ("chk-df-j", DFTYPE_INDIRECT, fn, dfkeyskl, DFKEY_COUNT);
  tlist = datafileGetList (tdf);
  vers = ilistGetVersion (tlist);
  ck_assert_int_eq (vers, 14);

  ilistStartIterator (list, &iteridx);
  while ((key = ilistIterateKey (list, &iteridx)) >= 0) {
    sval = ilistGetStr (list, key, 14);
    tsval = ilistGetStr (tlist, key, 14);
    ck_assert_str_eq (sval, tsval);
    nval = ilistGetNum (list, key, 15);
    tnval = ilistGetNum (tlist, key, 15);
    ck_assert_int_eq (nval, tnval);
  }

  datafileFree (df);
  datafileFree (tdf);
  unlink (fn);
}
END_TEST

START_TEST(datafile_simple_save)
{
  datafile_t *    df;
  datafile_t *    tdf;
  char            *val;
  char            *tval;
  char            *tstr = NULL;
  char            *fn;
  FILE            *fh;
  slist_t         *list;
  slist_t         *tlist;
  slistidx_t      iteridx;
  slistidx_t      titeridx;
  int             vers;


  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- datafile_simple_save");
  fn = "tmp/dftesto.txt";
  tstr = "# comment\n# version 15\nA\na\n# comment\nB\nb\nC\nc\nD\n# comment\nd\nE\ne\nF\nf\nG\n1200\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tstr);
  fclose (fh);

  df = datafileAllocParse ("chk-df-k", DFTYPE_LIST, fn, NULL, 0);
  unlink (fn);

  list = datafileGetList (df);
  vers = slistGetVersion (list);
  ck_assert_int_eq (vers, 15);

  fn = "tmp/dftestp.txt";
  datafileSaveList ("chk-df-k", fn, list);
  tdf = datafileAllocParse ("chk-df-k", DFTYPE_LIST, fn, NULL, 0);
  tlist = datafileGetList (tdf);

  slistStartIterator (list, &iteridx);
  slistStartIterator (tlist, &titeridx);
  while ((val = slistIterateKey (list, &iteridx)) != NULL) {
    tval = slistIterateKey (tlist, &titeridx);
    ck_assert_str_eq (val, tval);
  }

  datafileFree (df);
  datafileFree (tdf);
  unlink (fn);
}
END_TEST

Suite *
datafile_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("datafile");

  tc = tcase_create ("datafile-parse");
  tcase_set_tags (tc, "libbasic");
  tcase_add_test (tc, parse_init_free);
  tcase_add_test (tc, parse_simple);
  tcase_add_test (tc, parse_keyvalue);
  tcase_add_test (tc, parse_with_comments);
  suite_add_tcase (s, tc);

  tc = tcase_create ("datafile");
  tcase_set_tags (tc, "libbasic");
  tcase_add_test (tc, datafile_simple);
  tcase_add_test (tc, datafile_keyval_dfkey);
  tcase_add_test (tc, datafile_keyval_df_extra);
  tcase_add_test (tc, datafile_indirect);
  tcase_add_test (tc, datafile_indirect_missing);
  tcase_add_test (tc, datafile_keyval_savelist);
  tcase_add_test (tc, datafile_keyval_savebuffer);
  tcase_add_test (tc, datafile_keyval_save);
  tcase_add_test (tc, datafile_indirect_save);
  tcase_add_test (tc, datafile_simple_save);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
