#include "config.h"

#include <stdio.h>
#include <check.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "process.h"
#include "check_bdj.h"
#include "portability.h"

START_TEST(process_exists)
{
  pid_t     pid;

  pid = getpid ();
  int rc = processExists (pid);
  ck_assert_int_eq (rc, 0);
  rc = processExists (90876);
  ck_assert_int_lt (rc, 0);
}
END_TEST

Suite *
process_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Process Suite");
  tc = tcase_create ("Process");
  tcase_add_test (tc, process_exists);
  suite_add_tcase (s, tc);
  return s;
}

