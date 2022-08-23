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

#include "orgopt.h"
#include "check_bdj.h"
#include "slist.h"
#include "templateutil.h"

static void
setup (void)
{
  templateFileCopy ("orgopt.txt", "orgopt.txt");
}

START_TEST(orgopt_alloc)
{
  orgopt_t   *oopt;

  oopt = orgoptAlloc ();
  orgoptFree (oopt);
}
END_TEST

START_TEST(orgopt_chk)
{
  orgopt_t   *oopt;
  slist_t     *tlist;


  oopt = orgoptAlloc ();
  tlist = orgoptGetList (oopt);
  ck_assert_int_gt (slistGetCount (tlist), 0);
  orgoptFree (oopt);
}
END_TEST

Suite *
orgopt_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("orgopt");
  tc = tcase_create ("orgopt");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, orgopt_alloc);
  tcase_add_test (tc, orgopt_chk);
  suite_add_tcase (s, tc);
  return s;
}

