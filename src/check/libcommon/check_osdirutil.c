/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
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
#include "dirop.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "osdirutil.h"

START_TEST(osdirutil_chk)
{
  char    buff [BDJ4_PATH_MAX];
  char    nbuff [BDJ4_PATH_MAX];

  *buff = '\0';
  osGetCurrentDir (buff, sizeof (buff));
  ck_assert_int_ge (strlen (buff), 5);

  osChangeDir ("tmp");
  osGetCurrentDir (nbuff, sizeof (nbuff));
  ck_assert_int_eq (strlen (buff) + 4, strlen (nbuff));

  osChangeDir (buff);
  osGetCurrentDir (nbuff, sizeof (nbuff));
  ck_assert_str_eq (buff, nbuff);
}
END_TEST

Suite *
osdirutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("osdirutil");
  tc = tcase_create ("osdirutil");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, osdirutil_chk);
  suite_add_tcase (s, tc);

  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
