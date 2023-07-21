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

#include "bdjvarsdf.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"

START_TEST(bdjvarsdf_set_get)
{
  char    *data = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- bdjvarsdf_set_get");
  mdebugSubTag ("bdjvarsdf_set_get");

  bdjvarsdfSet (BDJVDF_DANCES, "test");
  data = bdjvarsdfGet (BDJVDF_DANCES);
  ck_assert_str_eq (data, "test");
}
END_TEST

Suite *
bdjvarsdf_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("bdjvarsdf");
  tc = tcase_create ("bdjvarsdf");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_test (tc, bdjvarsdf_set_get);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
