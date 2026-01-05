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

#include "bdjvarsdf.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"

START_TEST(bdjvarsdfload_chk)
{
  char    *data = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjvarsdfload_chk");
  mdebugSubTag ("bdjvarsdfload_chk");

  bdjvarsdfloadInit ();
  for (int i = 0; i < BDJVDF_MAX; ++i) {
    data = bdjvarsdfGet (i);
    ck_assert_ptr_nonnull (data);
  }
  bdjvarsdfloadCleanup ();
}
END_TEST

Suite *
bdjvarsdfload_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("bdjvarsdfload");
  tc = tcase_create ("bdjvarsdfload");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_test (tc, bdjvarsdfload_chk);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
