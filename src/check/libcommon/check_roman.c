/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "log.h"
#include "mdebug.h"
#include "roman.h"

typedef struct {
  int           value;
  int           retres;
  const char    *result;
} troman_t;

troman_t rtests [] = {
  { 1, 0, "I" },
  { 2, 0, "II" },
  { 3, 0, "III" },
  { 4, 0, "IV" },
  { 5, 0, "V" },
  { 6, 0, "VI" },
  { 7, 0, "VII" },
  { 8, 0, "VIII" },
  { 9, 0, "IX" },
  { 10, 0, "X" },
  { 11, 0, "XI" },
  { 12, 0, "XII" },
  { 13, 0, "XIII" },
  { 14, 0, "XIV" },
  { 15, 0, "XV" },
  { 16, 0, "XVI" },
  { 39, 0, "XXXIX" },
  { 400, 0, "CD" },
  { 40, 0, "XL" },
  { 90, 0, "XC" },
  { 900, 0, "CM" },
  { 246, 0, "CCXLVI" },
  { 789, 0, "DCCLXXXIX" },
  { 2421, 0, "MMCDXXI" },
  { 1009, 0, "MIX" },
  { 1066, 0, "MLXVI" },
};

enum {
  rtestsz = sizeof (rtests) / sizeof (troman_t),
};

START_TEST(roman)
{
  char  buff [20];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- roman");
  mdebugSubTag ("roman");

  for (int i = 0; i < rtestsz; ++i) {
    int   rc;

    rc = convertToRoman (rtests [i].value, buff, sizeof (buff));
    ck_assert_int_eq (rc, rtests [i].retres);
    ck_assert_str_eq (buff, rtests [i].result);
  }
}
END_TEST

Suite *
roman_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("roman");
  tc = tcase_create ("roman");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, roman);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
