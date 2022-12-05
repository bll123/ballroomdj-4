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
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "log.h"
#include "slist.h"
#include "sortopt.h"
#include "templateutil.h"

static void
setup (void)
{
  templateFileCopy ("sortopt.txt", "sortopt.txt");
}

START_TEST(sortopt_alloc)
{
  sortopt_t   *so;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- sortopt_alloc");

  so = sortoptAlloc ();
  sortoptFree (so);
}
END_TEST

START_TEST(sortopt_chk)
{
  sortopt_t   *so;
  slist_t     *tlist;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- sortopt_chk");


  so = sortoptAlloc ();
  tlist = sortoptGetList (so);
  ck_assert_int_gt (slistGetCount (tlist), 0);
  sortoptFree (so);
}
END_TEST

Suite *
sortopt_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("sortopt");
  tc = tcase_create ("sortopt");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, sortopt_alloc);
  tcase_add_test (tc, sortopt_chk);
  suite_add_tcase (s, tc);
  return s;
}

