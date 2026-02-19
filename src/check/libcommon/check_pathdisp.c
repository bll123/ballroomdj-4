/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "log.h"
#include "pathdisp.h"
#include "sysvars.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "sysvars.h"

START_TEST(path_disppath)
{
  char    to [BDJ4_PATH_MAX];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- path_disppath");
  mdebugSubTag ("path_disppath");

  stpecpy (to, to + sizeof (to), "/tmp/abc.txt");
  pathDisplayPath (to, sizeof (to));
  if (isWindows ()) {
    ck_assert_str_eq (to, "\\tmp\\abc.txt");
  } else {
    ck_assert_str_eq (to, "/tmp/abc.txt");
  }

  stpecpy (to, to + sizeof (to), "C:/tmp/abc.txt");
  pathDisplayPath (to, sizeof (to));
  if (isWindows ()) {
    ck_assert_str_eq (to, "C:\\tmp\\abc.txt");
  } else {
    ck_assert_str_eq (to, "C:/tmp/abc.txt");
  }
}
END_TEST

START_TEST(path_normpath)
{
  char    to [BDJ4_PATH_MAX];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- path_normpath");
  mdebugSubTag ("path_normpath");

  stpecpy (to, to + sizeof (to), "/tmp/abc.txt");
  pathNormalizePath (to, sizeof (to));
  ck_assert_str_eq (to, "/tmp/abc.txt");

  stpecpy (to, to + sizeof (to), "\\tmp\\abc.txt");
  pathNormalizePath (to, sizeof (to));
  if (isWindows ()) {
    ck_assert_str_eq (to, "/tmp/abc.txt");
  } else {
    ck_assert_str_eq (to, "\\tmp\\abc.txt");
  }

  stpecpy (to, to + sizeof (to), "C:/tmp/abc.txt");
  pathNormalizePath (to, sizeof (to));
  ck_assert_str_eq (to, "C:/tmp/abc.txt");

  stpecpy (to, to + sizeof (to), "C:\\tmp\\abc.txt");
  pathNormalizePath (to, sizeof (to));
  if (isWindows ()) {
    ck_assert_str_eq (to, "C:/tmp/abc.txt");
  } else {
    ck_assert_str_eq (to, "C:\\tmp\\abc.txt");
  }
}
END_TEST

Suite *
pathdisp_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("pathdisp");
  tc = tcase_create ("pathdisp");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, path_disppath);
  tcase_add_test (tc, path_normpath);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
