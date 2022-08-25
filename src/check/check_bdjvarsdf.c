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

#include "bdjvarsdf.h"
#include "check_bdj.h"

START_TEST(bdjvarsdf_set_get)
{
  char    *data = NULL;

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

