/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "bdjstring.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "osprocess.h"
#include "check_bdj.h"
#include "mdebug.h"
/* procutil hasn't had its tests run, but need the procutilExists routine */
#include "procutil.h"
#include "sysvars.h"

static void
runchk (int flags, int *rpid, int *rexists, int exitcode)
{
  char        tbuff [BDJ4_PATH_MAX];
  char        tmp [40];
  const char  *targv [10];
  int         targc = 0;
  int         pid = 0;
  procutil_t  process;
  char        *extension;
  int         rc;

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  snprintf (tbuff, sizeof (tbuff), "bin/chkprocess%s", extension);
  snprintf (tmp,sizeof (tmp), "%d", exitcode);

  targv [targc++] = tbuff;
  targv [targc++] = "--profile";
  targv [targc++] = "1";          // time to sleep
  targv [targc++] = "--bdj4";
  targv [targc++] = "--debug";
  targv [targc++] = tmp;
  targv [targc++] = NULL;
  pid = osProcessStart (targv, flags, NULL, NULL);
  process.pid = pid;
  process.hasHandle = false;
  rc = procutilExists (&process);
  *rpid = pid;
  *rexists = rc;
}

START_TEST(osprocess_start)
{
  int   pid;
  int   exists;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osprocess_start");
  mdebugSubTag ("osprocess_start");

  runchk (0, &pid, &exists, 0);
  ck_assert_int_gt (pid, 0);
  ck_assert_int_eq (exists, 0);
}
END_TEST

START_TEST(osprocess_start_detach)
{
  int   pid;
  int   exists;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osprocess_start_detach");
  mdebugSubTag ("osprocess_start_detach");

  runchk (OS_PROC_DETACH, &pid, &exists, 0);
  ck_assert_int_gt (pid, 0);
  ck_assert_int_eq (exists, 0);
}
END_TEST

START_TEST(osprocess_start_wait)
{
  int   rc;
  int   exists;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osprocess_start_wait");
  mdebugSubTag ("osprocess_start_wait");

  /* in this case, the pid is the return code from the program */

  runchk (OS_PROC_WAIT, &rc, &exists, 0);
  ck_assert_int_eq (rc, 0);

  runchk (OS_PROC_WAIT, &rc, &exists, 1);
  ck_assert_int_eq (rc, 1);

  runchk (OS_PROC_WAIT, &rc, &exists, 250);
  ck_assert_int_eq (rc, 250);
}
END_TEST

START_TEST(osprocess_start_handle)
{
  char        tbuff [BDJ4_PATH_MAX];
  const char  *targv [10];
  int         targc = 0;
  int         pid = 0;
  procutil_t  process;
  char        *extension;
  int         rc;
  int         flags = 0;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osprocess_start_handle");
  mdebugSubTag ("osprocess_start_handle");

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  snprintf (tbuff, sizeof (tbuff), "bin/chkprocess%s", extension);

  targv [targc++] = tbuff;
  targv [targc++] = "--profile";
  targv [targc++] = "1";          // time to sleep
  targv [targc++] = "--bdj4";
  targv [targc++] = NULL;
  process.processHandle = NULL;
  pid = osProcessStart (targv, flags, &process.processHandle, NULL);
  ck_assert_int_gt (pid, 0);
  if (process.processHandle != NULL) {
    process.hasHandle = true;
  }
  if (isWindows ()) {
    ck_assert_ptr_nonnull (process.processHandle);
  }
  process.pid = pid;
  rc = procutilExists (&process);
  ck_assert_int_eq (rc, 0);
#if _typ_HANDLE
  if (process.processHandle != NULL) {
    CloseHandle (process.processHandle);
  }
#endif
}
END_TEST

START_TEST(osprocess_start_redirect)
{
  char        tbuff [BDJ4_PATH_MAX];
  const char  *targv [10];
  int         targc = 0;
  int         pid = 0;
  char        *extension;
  int         flags = 0;
  char        *outfn = "tmp/chkosprocess.txt";
  FILE        *fh;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osprocess_start_redirect");
  mdebugSubTag ("osprocess_start_redirect");

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  snprintf (tbuff, sizeof (tbuff), "bin/chkprocess%s", extension);

  targv [targc++] = tbuff;
  targv [targc++] = "--profile";
  targv [targc++] = "0";          // time to sleep
  targv [targc++] = "--theme";
  targv [targc++] = "xyzzy";          // data to write
  targv [targc++] = "--bdj4";
  targv [targc++] = NULL;
  flags = OS_PROC_WAIT;
  pid = osProcessStart (targv, flags, NULL, outfn);
  /* waited, pid is the return code */
  ck_assert_int_eq (pid, 0);
  if (isWindows ()) {
    sleep (1);    // give windows a bit more time
  }
  ck_assert_int_eq (fileopFileExists (outfn), 1);
  fh = fileopOpen (outfn, "r");
  if (fh != NULL) {
    (void) ! fgets (tbuff, sizeof (tbuff), fh);
    mdextfclose (fh);
    fclose (fh);
  }
  stringTrim (tbuff);
  ck_assert_str_eq (tbuff, "xyzzy");
  fileopDelete (outfn);
}
END_TEST

START_TEST(osprocess_pipe)
{
  char        tbuff [BDJ4_PATH_MAX];
  char        pbuff [BDJ4_PATH_MAX];
  const char  *targv [10];
  int         targc = 0;
  int         rc = -2;
  char        *extension;
  int         flags = 0;
  size_t      retsz;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osprocess_pipe");
  mdebugSubTag ("osprocess_pipe");

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  snprintf (pbuff, sizeof (pbuff), "bin/chkprocess%s", extension);

  targv [targc++] = pbuff;
  targv [targc++] = "--profile";
  targv [targc++] = "0";          // time to sleep
  targv [targc++] = "--theme";
  targv [targc++] = "xyzzy";          // data to write
  targv [targc++] = "--bdj4";
  targv [targc++] = NULL;
  flags = OS_PROC_WAIT | OS_PROC_DETACH;           // must have wait on
  rc = osProcessPipe (targv, flags, tbuff, sizeof (tbuff), &retsz);
  ck_assert_int_eq (rc, 0);
  stringTrim (tbuff);
  ck_assert_str_eq (tbuff, "xyzzy");
  /* retsz still includes the trailing newline */
  if (isWindows ()) {
    ck_assert_int_eq (retsz, 7);
  } else {
    ck_assert_int_eq (retsz, 6);
  }
}
END_TEST

START_TEST(osprocess_pipe_wait)
{
  char        tbuff [BDJ4_PATH_MAX];
  char        pbuff [BDJ4_PATH_MAX];
  const char  *targv [10];
  int         targc = 0;
  int         rc = -2;
  char        *extension;
  int         flags = 0;
  size_t      retsz;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osprocess_pipe_wait");
  mdebugSubTag ("osprocess_pipe_wait");

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  snprintf (pbuff, sizeof (pbuff), "bin/chkprocess%s", extension);

  targv [targc++] = pbuff;
  targv [targc++] = "--profile";
  targv [targc++] = "3";          // time to sleep
  targv [targc++] = "--theme";
  targv [targc++] = "xyzzy";          // data to write
  targv [targc++] = "--bdj4";
  targv [targc++] = NULL;
  flags = OS_PROC_WAIT | OS_PROC_DETACH;           // must have wait on
  rc = osProcessPipe (targv, flags, tbuff, sizeof (tbuff), &retsz);
  ck_assert_int_eq (rc, 0);
  stringTrim (tbuff);
  ck_assert_str_eq (tbuff, "xyzzy");
  /* retsz still includes the trailing newline */
  if (isWindows ()) {
    ck_assert_int_eq (retsz, 7);
  } else {
    ck_assert_int_eq (retsz, 6);
  }

  /* again, without a buffer */
  rc = osProcessPipe (targv, flags, NULL, 0, NULL);
  ck_assert_int_eq (rc, 0);
}
END_TEST

START_TEST(osprocess_pipe_rc)
{
  char        pbuff [BDJ4_PATH_MAX];
  char        tbuff [BDJ4_PATH_MAX];
  const char  *targv [10];
  int         targc = 0;
  char        *extension;
  int         flags = 0;
  int         rcidx;
  size_t      retsz;
  int         rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osprocess_pipe_rc");
  mdebugSubTag ("osprocess_pipe_rc");

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  snprintf (pbuff, sizeof (pbuff), "bin/chkprocess%s", extension);

  targv [targc++] = pbuff;
  targv [targc++] = "--profile";
  targv [targc++] = "0";          // time to sleep
  targv [targc++] = "--theme";
  targv [targc++] = "xyzzy";      // data to write
  targv [targc++] = "--bdj4";
  targv [targc++] = "--debug";
  rcidx = targc;
  targv [targc++] = "0";          // return code
  targv [targc++] = NULL;

  flags = OS_PROC_WAIT | OS_PROC_DETACH;
  rc = osProcessPipe (targv, flags, tbuff, sizeof (tbuff), &retsz);
  stringTrim (tbuff);
  ck_assert_str_eq (tbuff, "xyzzy");
  ck_assert_int_eq (rc, 0);

  targv [rcidx] = "1";
  rc = osProcessPipe (targv, flags, tbuff, sizeof (tbuff), &retsz);
  stringTrim (tbuff);
  ck_assert_str_eq (tbuff, "xyzzy");
  ck_assert_int_eq (rc, 1);

  targv [rcidx] = "-5";
  rc = osProcessPipe (targv, flags, tbuff, sizeof (tbuff), &retsz);
  stringTrim (tbuff);
  ck_assert_str_eq (tbuff, "xyzzy");
  if (isWindows ()) {
    ck_assert_int_eq (rc, -5);
  } else {
    ck_assert_int_eq (rc, 256-5);
  }
}
END_TEST

START_TEST(osprocess_run)
{
  char        tbuff [BDJ4_PATH_MAX];
  char        *extension;
  char        *data;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osprocess_run");
  mdebugSubTag ("osprocess_run");

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  snprintf (tbuff, sizeof (tbuff), "bin/chkprocess%s", extension);

  data = osRunProgram (tbuff, "--profile", "0", "--theme", "xyzzy", "--bdj4", NULL);
  stringTrim (data);
  ck_assert_str_eq (data, "xyzzy");
  dataFree (data);
}
END_TEST


Suite *
osprocess_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("osprocess");
  tc = tcase_create ("osprocess");
  tcase_set_timeout (tc, 8.0);
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, osprocess_start);
  tcase_add_test (tc, osprocess_start_detach);
  tcase_add_test (tc, osprocess_start_wait);
  tcase_add_test (tc, osprocess_start_handle);
  tcase_add_test (tc, osprocess_start_redirect);
  tcase_add_test (tc, osprocess_pipe);
  tcase_add_test (tc, osprocess_pipe_wait);
  tcase_add_test (tc, osprocess_pipe_rc);
  tcase_add_test (tc, osprocess_run);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
