#include "config.h"
#include "configt.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>
#if _hdr_unistd
# include <unistd.h>
#endif

#include "filedata.h"
#include "check_bdj.h"
#include "portability.h"

START_TEST(file_readall)
{
  FILE    *fh;
  char    *data;

  char *fn = "tmp/abc.txt";
  fh = fopen (fn, "w");
  fclose (fh);
  /* empty file */
  data = filedataReadAll (fn);

  fh = fopen (fn, "w");
  fprintf (fh, "%s", "a");
  fclose (fh);
  /* one byte file */
  data = filedataReadAll (fn);
  ck_assert_int_eq (strlen (data), 1);
  ck_assert_mem_eq (data, "a", 1);

  char *tdata = "lkjsdflkdjsfljsdfljsdfd\n";
  fh = fopen (fn, "w");
  fprintf (fh, "%s", tdata);
  fclose (fh);
  data = filedataReadAll (fn);
  ck_assert_int_eq (strlen (data), strlen (tdata));
  ck_assert_mem_eq (data, tdata, strlen (tdata));
}
END_TEST

Suite *
filedata_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("File Data Utils Suite");
  tc = tcase_create ("File Data Utils");
  tcase_add_test (tc, file_readall);
  suite_add_tcase (s, tc);
  return s;
}

