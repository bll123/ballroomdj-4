#include "config.h"

#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

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

START_TEST(filedata_repl)
{
  FILE    *fh = NULL;
  char    *data = NULL;
  char    *tdata = NULL;
  size_t  len;
  size_t  dlen;
  char    *ndata = NULL;
  char    *ndatachk1 = NULL;
  char    *ndatachk2 = NULL;
  char    *ndatachk3 = NULL;
  char    *ndatachk4 = NULL;
  char    *ndatachk5 = NULL;
  char    *ndatachk6 = NULL;
  char    *fn = "tmp/abc.txt";

      tdata = "abc123def456ghi789qwertyzzz123def456ghi789";
  ndatachk1 = "abc123def456ghi789ASDFGHzzz123def456ghi789";
  ndatachk2 = "abc123def456ghi789ABCzzz123def456ghi789";
  ndatachk3 = "abc123def456ghi789ABCDEFGHIJKLzzz123def456ghi789";
  ndatachk4 = "abc123defABCghi789qwertyzzz123defABCghi789";
  ndatachk5 = "abcZdef456ghi789qwertyzzzZdef456ghi789";
  ndatachk6 = "abcVWXYZdef456ghi789qwertyzzzVWXYZdef456ghi789";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tdata);
  fclose (fh);
  data = filedataReadAll (fn, &dlen);
  ck_assert_int_eq (strlen (data), dlen);
  ck_assert_mem_eq (data, tdata, strlen (tdata));

  /* equal length : 1 */
  len = dlen;
  ndata = filedataReplace (data, &len, "qwerty", "ASDFGH");
  ck_assert_int_eq (strlen (ndatachk1), len);
  ck_assert_mem_eq (ndata, ndatachk1, len);
  free (ndata);

  /* shorter length : 1 */
  len = dlen;
  ndata = filedataReplace (data, &len, "qwerty", "ABC");
  ck_assert_int_eq (strlen (ndatachk2), len);
  ck_assert_mem_eq (ndata, ndatachk2, len);
  free (ndata);

  /* longer length : 1 */
  len = dlen;
  ndata = filedataReplace (data, &len, "qwerty", "ABCDEFGHIJKL");
  ck_assert_int_eq (strlen (ndatachk3), len);
  ck_assert_mem_eq (ndata, ndatachk3, len);
  free (ndata);

  /* equal length : multiple */
  len = dlen;
  ndata = filedataReplace (data, &len, "456", "ABC");
  ck_assert_int_eq (strlen (ndatachk4), len);
  ck_assert_mem_eq (ndata, ndatachk4, len);
  free (ndata);

  /* shorter length : multiple */
  len = dlen;
  ndata = filedataReplace (data, &len, "123", "Z");
  ck_assert_int_eq (strlen (ndatachk5), len);
  ck_assert_mem_eq (ndata, ndatachk5, len);
  free (ndata);

  /* longer length : multiple */
  len = dlen;
  ndata = filedataReplace (data, &len, "123", "VWXYZ");
  ck_assert_int_eq (strlen (ndatachk6), len);
  ck_assert_mem_eq (ndata, ndatachk6, len);
  free (ndata);

  free (data);
  unlink (fn);
}
END_TEST

Suite *
filedata_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("File Data Utils Suite");
  tc = tcase_create ("File Data Utils");
  tcase_add_test (tc, filedata_readall);
  tcase_add_test (tc, filedata_repl);
  suite_add_tcase (s, tc);
  return s;
}

