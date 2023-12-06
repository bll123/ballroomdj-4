/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "audiosrc.h"
#include "bdjopt.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"

typedef struct {
  char  *test;
  char  result [1024];
} chk_audsrc_t;

static chk_audsrc_t tvalues [] = {
  { "abc123", "/testpath/abc123" },
  { "/stuff", "/stuff" },
  { "C:/there", "C:/there" },
  { "d:/here", "d:/here" }
};
enum {
  tvaluesz = sizeof (tvalues) / sizeof (chk_audsrc_t),
};

START_TEST(audiosrc_gettype)
{
  int   rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- audiosrc_gettype");
  mdebugSubTag ("audiosrc_gettype");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "/testpath");
  for (int i = 0; i < tvaluesz; ++i) {
    rc = audiosrcGetType (tvalues [i].test);
    ck_assert_int_eq (rc, AUDIOSRC_TYPE_FILE);
  }
  bdjoptCleanup ();
}
END_TEST

START_TEST(audiosrc_fullpath)
{
  char  *val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- audiosrc_fullpath");
  mdebugSubTag ("audiosrc_fullpath");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "/testpath");
  for (int i = 0; i < tvaluesz; ++i) {
    audiosrcFullPath (tvalues [i].test, tvalues [i].result, sizeof (tvalues [i].result));
    ck_assert_str_eq (val, tvalues [i].result);
  }
  bdjoptCleanup ();
}
END_TEST

Suite *
audiosrc_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("audiosrc");
  tc = tcase_create ("audiosrc");
  tcase_set_tags (tc, "libaudiosrc");
  tcase_add_test (tc, audiosrc_gettype);
  tcase_add_test (tc, audiosrc_fullpath);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
