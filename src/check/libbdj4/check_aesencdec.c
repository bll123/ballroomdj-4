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
#include "log.h"
#include "aesencdec.h"

typedef struct {
  const char  *d;
} aesec_t;

static aesec_t tests [] = {
  { "abcdef", },
  { "12345693", },
  { ".,mdfvda", },
};

enum {
  TEST_SZ = sizeof (tests) / sizeof (aesec_t),
};

START_TEST(aesencdec_encdec)
{
  char  dbuff [300];
  char  ebuff [300];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- aesencdec_encdec");
  mdebugSubTag ("aesencdec_encdec");

  for (int i = 0; i < TEST_SZ; ++i) {
    aesencrypt (tests [i].d, ebuff, sizeof (ebuff));
    aesdecrypt (ebuff, dbuff, sizeof (dbuff));
    ck_assert_str_eq (tests [i].d, dbuff);
  }
}
END_TEST


Suite *
aesencdec_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("aesencdec");
  tc = tcase_create ("aesencdec");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, aesencdec_encdec);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
