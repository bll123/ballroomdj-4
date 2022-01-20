#include "config.h"
#include "configt.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <check.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "check_bdj.h"
#include "pathbld.h"
#include "portability.h"
#include "process.h"
#include "sysvars.h"

START_TEST(process_exists)
{
  pid_t     pid;
  int       rc;

  pid = getpid ();
  rc = processExists (pid);
  ck_assert_int_eq (rc, 0);
  rc = processExists (90876);
  ck_assert_int_lt (rc, 0);
}
END_TEST

START_TEST(process_start)
{
  pid_t     ppid;
  pid_t     cpid;
  int       rc;
  char      *extension;
  char      tbuff [MAXPATHLEN];

  ppid = getpid ();

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  pathbldMakePath (tbuff, sizeof (tbuff), "",
      "chkprocess", extension, PATHBLD_MP_EXECDIR);
  /* if the signal is not ignored, the process goes into a zombie state */
  /* and still exists */
#if _define_SIGCHLD
  processIgnoreSignal (SIGCHLD);
#endif
  rc = processStart (tbuff, &cpid, 5, 0);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ne (ppid, cpid);
  rc = processExists (cpid);
  ck_assert_int_eq (rc, 0);
  sleep (6);
  rc = processExists (cpid);
  ck_assert_int_ne (rc, 0);
}
END_TEST

START_TEST(process_kill)
{
  pid_t     ppid;
  pid_t     cpid;
  int       rc;
  char      *extension;
  char      tbuff [MAXPATHLEN];

  ppid = getpid ();

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  pathbldMakePath (tbuff, sizeof (tbuff), "",
      "chkprocess", extension, PATHBLD_MP_EXECDIR);
  /* if the signal is not ignored, the process goes into a zombie state */
  /* and still exists */
#if _define_SIGCHLD
  processIgnoreSignal (SIGCHLD);
#endif
  rc = processStart (tbuff, &cpid, 60, 0);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ne (ppid, cpid);
  rc = processExists (cpid);
  ck_assert_int_eq (rc, 0);
  sleep (2);
  rc = processExists (cpid);
  ck_assert_int_eq (rc, 0);
  rc = processKill (cpid);
  ck_assert_int_eq (rc, 0);
  sleep (2);
  rc = processExists (cpid);
  ck_assert_int_ne (rc, 0);
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
  tcase_add_test (tc, process_start);
  tcase_add_test (tc, process_kill);
  tcase_set_timeout (tc, 10.0);
  suite_add_tcase (s, tc);
  return s;
}

