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
#include <signal.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdj4.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "fileop.h"
#include "fileshared.h"
#include "log.h"
#include "ossignal.h"
#include "pathbld.h"
#include "procutil.h"
#include "sysvars.h"

#define FN  "tmp/fileshared.txt"
#define DATAA "abc123\n"
#define DATAB "def456\n"  // must be the same as in chkfileshared.c

START_TEST(fileshared_open_trunc)
{
  fileshared_t  *sfh;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_open_trunc");
  mdebugSubTag ("fileshared_open_trunc");

  sfh = fileSharedOpen (FN, FILE_OPEN_TRUNCATE);
  ck_assert_ptr_nonnull (sfh);
  fileSharedClose (sfh);
  unlink (FN);
}
END_TEST

START_TEST(fileshared_open_append)
{
  fileshared_t  *sfh;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_open_append");
  mdebugSubTag ("fileshared_open_append");

  sfh = fileSharedOpen (FN, FILE_OPEN_APPEND);
  ck_assert_ptr_nonnull (sfh);
  fileSharedClose (sfh);
  sfh = fileSharedOpen (FN, FILE_OPEN_APPEND);
  ck_assert_ptr_nonnull (sfh);
  fileSharedClose (sfh);
  unlink (FN);
}
END_TEST

START_TEST(fileshared_write)
{
  fileshared_t  *sfh;
  size_t        sz;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_write");
  mdebugSubTag ("fileshared_write");

  sfh = fileSharedOpen (FN, FILE_OPEN_TRUNCATE);
  fileSharedWrite (sfh, DATAA, strlen (DATAA));
  fileSharedClose (sfh);
  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA));
  unlink (FN);
}
END_TEST


START_TEST(fileshared_write_append)
{
  fileshared_t  *sfh;
  size_t        sz;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_write_append");
  mdebugSubTag ("fileshared_write_append");

  sfh = fileSharedOpen (FN, FILE_OPEN_TRUNCATE);
  fileSharedWrite (sfh, DATAA, strlen (DATAA));
  fileSharedClose (sfh);
  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA));

  sfh = fileSharedOpen (FN, FILE_OPEN_APPEND);
  fileSharedWrite (sfh, DATAA, strlen (DATAA));
  fileSharedClose (sfh);
  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA) * 2);
  unlink (FN);
}
END_TEST

START_TEST(fileshared_write_multiple)
{
  fileshared_t  *sfha;
  fileshared_t  *sfhb;
  size_t        sz;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_write_multiple");
  mdebugSubTag ("fileshared_write_multiple");

  sfha = fileSharedOpen (FN, FILE_OPEN_APPEND);
  fileSharedWrite (sfha, DATAA, strlen (DATAA));

  sfhb = fileSharedOpen (FN, FILE_OPEN_APPEND);
  ck_assert_ptr_nonnull (sfhb);
  fileSharedWrite (sfhb, DATAA, strlen (DATAA));

  fileSharedWrite (sfha, DATAA, strlen (DATAA));
  fileSharedWrite (sfhb, DATAA, strlen (DATAA));

  fileSharedClose (sfha);
  fileSharedClose (sfhb);

  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA) * 4);
  unlink (FN);
}
END_TEST

START_TEST(fileshared_write_shared)
{
  fileshared_t  *sfh;
  size_t        sz;
  char          *extension;
  char          tbuff [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_write_shared");
  mdebugSubTag ("fileshared_write_shared");

  sfh = fileSharedOpen (FN, FILE_OPEN_APPEND);
  fileSharedWrite (sfh, DATAA, strlen (DATAA));

  extension = "";
  if (isWindows ()) {
    extension = ".exe";
  }
  pathbldMakePath (tbuff, sizeof (tbuff),
      "chkfileshared", extension, PATHBLD_MP_DIR_EXEC);
  /* if the signal is not ignored, the process goes into a zombie state */
  /* and still exists */
#if _define_SIGCHLD
  osIgnoreSignal (SIGCHLD);
#endif
  for (int i = 0; i < 40; ++i) {
    procutil_t    *process;

    process = procutilStart (tbuff, 0, 0, PROCUTIL_NO_DETACH, NULL);
    ck_assert_ptr_nonnull (process);
    fileSharedWrite (sfh, DATAA, strlen (DATAA));
    procutilFree (process);
  }
  sleep (2);

  fileSharedClose (sfh);

  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA) * 41 + strlen (DATAB) * 40);
  unlink (FN);
}
END_TEST


Suite *
fileshared_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("fileshared");
  tc = tcase_create ("fileshared");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, fileshared_open_trunc);
  tcase_add_test (tc, fileshared_open_append);
  tcase_add_test (tc, fileshared_write);
  tcase_add_test (tc, fileshared_write_append);
  tcase_add_test (tc, fileshared_write_multiple);
  tcase_add_test (tc, fileshared_write_shared);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
