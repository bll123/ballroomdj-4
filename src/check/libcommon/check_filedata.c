/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "mdebug.h"
#include "fileop.h"
#include "filedata.h"
#include "log.h"
#include "mdebug.h"

START_TEST(filedata_readall)
{
  FILE    *fh;
  char    *data;
  char    *tdata;
  size_t  len;
  char *fn = "tmp/abc.txt";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- filedata_readall");
  mdebugSubTag ("filedata_readall");

  fh = fileopOpen (fn, "w");
  mdextfclose (fh);
  fclose (fh);
  /* empty file */
  data = filedataReadAll (fn, &len);
  ck_assert_int_eq (len, 0);
  ck_assert_ptr_null (data);
  dataFree (data);

  fh = fileopOpen (fn, "w");
  fprintf (fh, "%s", "a");
  mdextfclose (fh);
  fclose (fh);
  /* one byte file */
  data = filedataReadAll (fn, &len);
  ck_assert_int_eq (len, 1);
  ck_assert_int_eq (strlen (data), 1);
  ck_assert_mem_eq (data, "a", 1);
  mdfree (data);

  tdata = "lkjsdflkdjsfljsdfljsdfd\n";
  fh = fileopOpen (fn, "w");
  fprintf (fh, "%s", tdata);
  mdextfclose (fh);
  fclose (fh);
  data = filedataReadAll (fn, &len);
  ck_assert_int_eq (len, strlen (tdata));
  ck_assert_int_eq (strlen (data), strlen (tdata));
  ck_assert_mem_eq (data, tdata, strlen (tdata));
  mdfree (data);

  unlink (fn);
}
END_TEST

Suite *
filedata_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("filedata");
  tc = tcase_create ("filedata");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, filedata_readall);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
