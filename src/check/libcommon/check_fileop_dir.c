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
#include <sys/stat.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "mdebug.h"
#include "dirop.h"
#include "fileop.h"
#include "log.h"
#include "osutils.h"
#include "sysvars.h"

START_TEST(fileop_is_dir)
{
  FILE      *fh;
  int       rc;
  char *fn = "tmp/d-abc";
  char *fnb = "tmp/d-def";
  char *fnc = "tmp/sl-abc";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileop_is_dir");
  mdebugSubTag ("fileop_is_dir");

  unlink (fnb);
  unlink (fnc);
  diropDeleteDir (fn, DIROP_ALL);
  rc = fileopIsDirectory (fn);
  ck_assert_int_eq (rc, 0);

  diropMakeDir (fn);

  fh = fileopOpen (fnb, "w");
  mdextfclose (fh);
  fclose (fh);
  rc = osCreateLink ("d-abc", fnc);

  rc = fileopIsDirectory (fn);
  ck_assert_int_eq (rc, 1);

  rc = fileopIsDirectory (fnb);
  ck_assert_int_eq (rc, 0);

  if (! isWindows ()) {
    rc = fileopIsDirectory (fnc);
    ck_assert_int_eq (rc, 1);
    rc = osIsLink (fnc);
    ck_assert_int_eq (rc, 1);
  }

  diropDeleteDir (fn, DIROP_ALL);

  rc = fileopIsDirectory (fn);
  ck_assert_int_eq (rc, 0);

  unlink (fnb);
  unlink (fnc);
}
END_TEST


Suite *
fileop_dir_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("fileop_dir");
  tc = tcase_create ("fileop_dir");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, fileop_is_dir);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
