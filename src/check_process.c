
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
#include "tmutil.h"

START_TEST(process_exists)
{
  pid_t     pid;
  int       rc;
  process_t process;

  pid = getpid ();
  process.pid = pid;
  process.hasHandle = false;
  rc = processExists (&process);
  ck_assert_int_eq (rc, 0);
  process.pid = 90876;
  process.hasHandle = false;
  rc = processExists (&process);
  ck_assert_int_lt (rc, 0);
}
END_TEST

START_TEST(process_start)
{
  pid_t     ppid;
  int       rc;
  char      *extension;
  char      tbuff [MAXPATHLEN];
  process_t *process;

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
  /* abuse the --profile argument to pass the seconds to sleep */
  process = processStart (tbuff, 5, 0);
  ck_assert_ptr_nonnull (process);
  ck_assert_int_ne (ppid, process->pid);
  rc = processExists (process);
  ck_assert_int_eq (rc, 0);
  mssleep (6000);
  rc = processExists (process);
  ck_assert_int_ne (rc, 0);
  processFree (process);
}
END_TEST

START_TEST(process_kill)
{
  pid_t     ppid;
  int       rc;
  char      *extension;
  char      tbuff [MAXPATHLEN];
  process_t *process;

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
  /* abuse the --profile argument to pass the seconds to sleep */
  process = processStart (tbuff, 60, 0);
  ck_assert_ptr_nonnull (process);
  ck_assert_int_ne (ppid, process->pid);

  rc = processExists (process);
  ck_assert_int_eq (rc, 0);

  mssleep (2000);
  rc = processExists (process);
  ck_assert_int_eq (rc, 0);

  mssleep (2000);
  rc = processExists (process);
  ck_assert_int_eq (rc, 0);

  rc = processKill (process);
  ck_assert_int_eq (rc, 0);

  mssleep (2000);
  rc = processExists (process);
  ck_assert_int_ne (rc, 0);

  processFree (process);
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

