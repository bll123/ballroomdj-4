/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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

#include "bdj4.h"
#include "check_bdj.h"
#include "dylib.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "sysvars.h"

START_TEST(dylib_load)
{
  dlhandle_t  *dlh;
  const char  *i18nnm = "libicui18n";
  const char  *pkgnm;
  char        dlpath [BDJ4_PATH_MAX];

  if (isWindows ()) {
    i18nnm = "libicuin";
  }

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dylib_load");
  mdebugSubTag ("dylib_load");

  pkgnm = "libatibdj4";
  pathbldMakePath (dlpath, sizeof (dlpath),
      pkgnm, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
  dlh = dylibLoad (dlpath, DYLIB_OPT_NONE);
  ck_assert_ptr_nonnull (dlh);
  dylibClose (dlh);

  dlh = dylibLoad (i18nnm,
      DYLIB_OPT_MAC_PREFIX | DYLIB_OPT_VERSION | DYLIB_OPT_ICU);
  ck_assert_ptr_nonnull (dlh);
  dylibClose (dlh);
}
END_TEST

Suite *
dylib_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("dylib");
  tc = tcase_create ("dylib");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, dylib_load);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
