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

#include "filedata.h"
#include "check_bdj.h"

START_TEST(filedata_readall)
{
  FILE    *fh;
  char    *data;
  char    *tdata;
  size_t  len;

  char *fn = "tmp/abc.txt";
  fh = fopen (fn, "w");
  fclose (fh);
  /* empty file */
  data = filedataReadAll (fn, &len);
  ck_assert_int_eq (len, 0);
  ck_assert_ptr_null (data);
  free (data);

  fh = fopen (fn, "w");
  fprintf (fh, "%s", "a");
  fclose (fh);
  /* one byte file */
  data = filedataReadAll (fn, &len);
  ck_assert_int_eq (len, 1);
  ck_assert_int_eq (strlen (data), 1);
  ck_assert_mem_eq (data, "a", 1);
  free (data);

  tdata = "lkjsdflkdjsfljsdfljsdfd\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tdata);
  fclose (fh);
  data = filedataReadAll (fn, &len);
  ck_assert_int_eq (len, strlen (tdata));
  ck_assert_int_eq (strlen (data), strlen (tdata));
  ck_assert_mem_eq (data, tdata, strlen (tdata));
  free (data);

  unlink (fn);
}
END_TEST

typedef struct {
  char  *str;
  char  *repl;
  char  *result;
} chk_filedata_t;

static char *teststr = "abc123def456ghi789qwertyzzz123def456ghi789";
static chk_filedata_t tvalues [] = {
  { "qwerty", "ASDFGH", "abc123def456ghi789ASDFGHzzz123def456ghi789" },
  { "qwerty", "ABC", "abc123def456ghi789ABCzzz123def456ghi789" },
  { "qwerty", "ABCDEFGHIJKL", "abc123def456ghi789ABCDEFGHIJKLzzz123def456ghi789" },
  { "456", "ABC", "abc123defABCghi789qwertyzzz123defABCghi789" },
  { "123", "Z", "abcZdef456ghi789qwertyzzzZdef456ghi789" },
  { "123", "VWXYZ", "abcVWXYZdef456ghi789qwertyzzzVWXYZdef456ghi789" }
};
enum {
  tvaluesz = sizeof (tvalues) / sizeof (chk_filedata_t),
};

START_TEST(filedata_repl)
{
  char    *data = NULL;
  size_t  len;
  char    *ndata = NULL;

  for (int i = 0; i < tvaluesz; ++i) {
    data = teststr;
    len = strlen (data);
    ndata = filedataReplace (data, &len, tvalues [i].str, tvalues [i].repl);
    ck_assert_int_eq (strlen (tvalues [i].result), len);
    ck_assert_mem_eq (ndata, tvalues [i].result, len);
    free (ndata);
  }
}
END_TEST

Suite *
filedata_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("filedata");
  tc = tcase_create ("filedata");
  tcase_add_test (tc, filedata_readall);
  tcase_add_test (tc, filedata_repl);
  suite_add_tcase (s, tc);
  return s;
}

