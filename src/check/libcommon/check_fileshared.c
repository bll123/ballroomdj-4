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
#include "tmutil.h"

#define FN  "tmp/fileshared.txt"
#define DATAA "abc123\n"
#define DATAB "def456xyz\n"  // must be the same as in chkfileshared.c
#define DATAC "# Do not edit this file.\n"

START_TEST(fileshared_open_trunc)
{
  fileshared_t  *sfh;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_open_trunc");
  mdebugSubTag ("fileshared_open_trunc");

  sfh = fileSharedOpen (FN, FILESH_OPEN_TRUNCATE, FILESH_FLUSH);
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

  sfh = fileSharedOpen (FN, FILESH_OPEN_APPEND, FILESH_FLUSH);
  ck_assert_ptr_nonnull (sfh);
  fileSharedClose (sfh);
  sfh = fileSharedOpen (FN, FILESH_OPEN_APPEND, FILESH_FLUSH);
  ck_assert_ptr_nonnull (sfh);
  fileSharedClose (sfh);
  unlink (FN);
}
END_TEST

START_TEST(fileshared_open_read_write)
{
  fileshared_t  *sfh;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_open_read_write");
  mdebugSubTag ("fileshared_open_read_write");

  sfh = fileSharedOpen (FN, FILESH_OPEN_READ_WRITE, FILESH_FLUSH);
  ck_assert_ptr_nonnull (sfh);
  fileSharedClose (sfh);
  sfh = fileSharedOpen (FN, FILESH_OPEN_READ_WRITE, FILESH_FLUSH);
  ck_assert_ptr_nonnull (sfh);
  fileSharedClose (sfh);
  sfh = fileSharedOpen (FN, FILESH_OPEN_READ, FILESH_FLUSH);
  ck_assert_ptr_nonnull (sfh);
  fileSharedClose (sfh);
  unlink (FN);
}
END_TEST

START_TEST(fileshared_open_read)
{
  fileshared_t  *sfh;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_open_read");
  mdebugSubTag ("fileshared_open_read");

  sfh = fileSharedOpen (FN, FILESH_OPEN_READ_WRITE, FILESH_FLUSH);
  ck_assert_ptr_nonnull (sfh);
  fileSharedClose (sfh);
  sfh = fileSharedOpen (FN, FILESH_OPEN_READ, FILESH_FLUSH);
  ck_assert_ptr_nonnull (sfh);
  fileSharedClose (sfh);
  sfh = fileSharedOpen (FN, FILESH_OPEN_READ, FILESH_FLUSH);
  ck_assert_ptr_nonnull (sfh);
  fileSharedClose (sfh);
  unlink (FN);
}
END_TEST

START_TEST(fileshared_write_trunc)
{
  fileshared_t  *sfh;
  size_t        sz;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_write_trunc");
  mdebugSubTag ("fileshared_write_trunc");

  sfh = fileSharedOpen (FN, FILESH_OPEN_TRUNCATE, FILESH_FLUSH);
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

  sfh = fileSharedOpen (FN, FILESH_OPEN_TRUNCATE, FILESH_FLUSH);
  fileSharedWrite (sfh, DATAA, strlen (DATAA));
  fileSharedClose (sfh);
  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA));

  sfh = fileSharedOpen (FN, FILESH_OPEN_APPEND, FILESH_FLUSH);
  fileSharedWrite (sfh, DATAB, strlen (DATAB));
  fileSharedClose (sfh);
  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA) + strlen (DATAB));
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

  sfha = fileSharedOpen (FN, FILESH_OPEN_APPEND, FILESH_FLUSH);
  fileSharedWrite (sfha, DATAA, strlen (DATAA));

  sfhb = fileSharedOpen (FN, FILESH_OPEN_APPEND, FILESH_FLUSH);
  ck_assert_ptr_nonnull (sfhb);
  fileSharedWrite (sfhb, DATAA, strlen (DATAA));

  fileSharedWrite (sfha, DATAB, strlen (DATAB));
  fileSharedWrite (sfhb, DATAB, strlen (DATAB));

  fileSharedClose (sfha);
  fileSharedClose (sfhb);

  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA) * 2 + strlen (DATAB) * 2);
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

  sfh = fileSharedOpen (FN, FILESH_OPEN_APPEND, FILESH_FLUSH);
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
  mssleep (1500);   /* must be longer than the sleeps in chkfileshared.c */

  fileSharedClose (sfh);

  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA) * 41 + strlen (DATAB) * 40);
  unlink (FN);
}
END_TEST

START_TEST(fileshared_read)
{
  fileshared_t  *sfh;
  size_t        sz;
  char          tbuff [40];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_read");
  mdebugSubTag ("fileshared_read");

  sfh = fileSharedOpen (FN, FILESH_OPEN_TRUNCATE, FILESH_FLUSH);
  fileSharedWrite (sfh, DATAA, strlen (DATAA));
  fileSharedWrite (sfh, DATAB, strlen (DATAB));
  fileSharedClose (sfh);
  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA) + strlen (DATAB));

  sfh = fileSharedOpen (FN, FILESH_OPEN_READ, FILESH_FLUSH);
  sz = fileSharedRead (sfh, tbuff, strlen (DATAA));
  ck_assert_int_eq (sz, 1);
  ck_assert_mem_eq (tbuff, DATAA, strlen (DATAA));
  sz = fileSharedRead (sfh, tbuff, strlen (DATAB));
  ck_assert_int_eq (sz, 1);
  ck_assert_mem_eq (tbuff, DATAB, strlen (DATAB));
  sz = fileSharedRead (sfh, tbuff, strlen (DATAA));
  ck_assert_int_eq (sz, 0);
  fileSharedClose (sfh);

  unlink (FN);
}
END_TEST

START_TEST(fileshared_seek)
{
  fileshared_t  *sfh;
  ssize_t       sz;
  int           rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_seek");
  mdebugSubTag ("fileshared_seek");

  sfh = fileSharedOpen (FN, FILESH_OPEN_TRUNCATE, FILESH_FLUSH);
  fileSharedWrite (sfh, DATAA, strlen (DATAA));
  fileSharedWrite (sfh, DATAB, strlen (DATAB));
  fileSharedClose (sfh);
  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA) + strlen (DATAB));

  sfh = fileSharedOpen (FN, FILESH_OPEN_READ, FILESH_FLUSH);
  rc = fileSharedSeek (sfh, 0, SEEK_SET);
  ck_assert_int_eq (rc, 0);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, 0);
  rc = fileSharedSeek (sfh, strlen (DATAA), SEEK_SET);
  ck_assert_int_eq (rc, 0);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA));
  rc = fileSharedSeek (sfh, strlen (DATAA) + strlen (DATAB), SEEK_SET);
  ck_assert_int_eq (rc, 0);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA) + strlen (DATAB));

  rc = fileSharedSeek (sfh, 0, SEEK_SET);
  ck_assert_int_eq (rc, 0);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, 0);
  rc = fileSharedSeek (sfh, strlen (DATAA), SEEK_CUR);
  ck_assert_int_eq (rc, 0);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA));
  rc = fileSharedSeek (sfh, strlen (DATAB), SEEK_CUR);
  ck_assert_int_eq (rc, 0);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA) + strlen (DATAB));

  rc = fileSharedSeek (sfh, 0, SEEK_END);
  ck_assert_int_eq (rc, 0);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA) + strlen (DATAB));
  rc = fileSharedSeek (sfh, - strlen (DATAB), SEEK_CUR);
  ck_assert_int_eq (rc, 0);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA));
  rc = fileSharedSeek (sfh, - strlen (DATAA), SEEK_CUR);
  ck_assert_int_eq (rc, 0);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, 0);
  rc = fileSharedSeek (sfh, - strlen (DATAA), SEEK_CUR);
  ck_assert_int_lt (rc, 0);

  rc = fileSharedSeek (sfh, - strlen (DATAB), SEEK_END);
  ck_assert_int_eq (rc, 0);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA));
  rc = fileSharedSeek (sfh, - strlen (DATAA), SEEK_CUR);
  ck_assert_int_eq (rc, 0);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, 0);

  fileSharedClose (sfh);

  unlink (FN);
}
END_TEST

START_TEST(fileshared_read_write)
{
  fileshared_t  *sfh;
  size_t        sz;
  char          tbuff [40];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_read_write");
  mdebugSubTag ("fileshared_read_write");

  sfh = fileSharedOpen (FN, FILESH_OPEN_READ_WRITE, FILESH_FLUSH);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, 0);
  fileSharedWrite (sfh, DATAA, strlen (DATAA));
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA));
  fileSharedWrite (sfh, DATAB, strlen (DATAB));
  fileSharedClose (sfh);
  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA) + strlen (DATAB));

  sfh = fileSharedOpen (FN, FILESH_OPEN_READ_WRITE, FILESH_FLUSH);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, 0);
  fileSharedWrite (sfh, DATAB, strlen (DATAB));
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAB));
  fileSharedWrite (sfh, DATAA, strlen (DATAA));
  fileSharedClose (sfh);
  sz = fileopSize (FN);
  ck_assert_int_eq (sz, strlen (DATAA) + strlen (DATAB));

  sfh = fileSharedOpen (FN, FILESH_OPEN_READ_WRITE, FILESH_FLUSH);
  fileSharedSeek (sfh, 0, SEEK_SET);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, 0);
  fileSharedFlush (sfh, FILESH_NO_SYNC);
  sz = fileSharedRead (sfh, tbuff, strlen (DATAB));
  ck_assert_int_eq (sz, 1);
  ck_assert_mem_eq (tbuff, DATAB, strlen (DATAB));
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAB));
  fileSharedSeek (sfh, strlen (DATAB), SEEK_SET);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAB));
  fileSharedFlush (sfh, FILESH_NO_SYNC);
  sz = fileSharedRead (sfh, tbuff, strlen (DATAA));
  ck_assert_int_eq (sz, 1);
  ck_assert_mem_eq (tbuff, DATAA, strlen (DATAA));
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA) + strlen (DATAB));
  fileSharedClose (sfh);

  sfh = fileSharedOpen (FN, FILESH_OPEN_READ_WRITE, FILESH_FLUSH);
  fileSharedSeek (sfh, 0, SEEK_SET);
  fileSharedWrite (sfh, DATAA, strlen (DATAA));
  fileSharedFlush (sfh, FILESH_SYNC);
  fileSharedSeek (sfh, 0, SEEK_SET);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, 0);
  fileSharedFlush (sfh, FILESH_NO_SYNC);
  sz = fileSharedRead (sfh, tbuff, strlen (DATAA));
  fileSharedFlush (sfh, FILESH_NO_SYNC);
  ck_assert_int_eq (sz, 1);
  ck_assert_mem_eq (tbuff, DATAA, strlen (DATAA));
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA));
  fileSharedSeek (sfh, strlen (DATAA), SEEK_SET);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA));
  fileSharedWrite (sfh, DATAB, strlen (DATAB));
  fileSharedFlush (sfh, FILESH_SYNC);
  fileSharedSeek (sfh, strlen (DATAA), SEEK_SET);
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA));
  fileSharedFlush (sfh, FILESH_NO_SYNC);
  sz = fileSharedRead (sfh, tbuff, strlen (DATAB));
  fileSharedFlush (sfh, FILESH_NO_SYNC);
  ck_assert_int_eq (sz, 1);
  ck_assert_mem_eq (tbuff, DATAB, strlen (DATAB));
  sz = fileSharedTell (sfh);
  ck_assert_int_eq (sz, strlen (DATAA) + strlen (DATAB));
  fileSharedClose (sfh);

  unlink (FN);
}
END_TEST

START_TEST(fileshared_get)
{
  fileshared_t  *sfh;
  char          tbuff [90];
  char          *p;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- fileshared_get");
  mdebugSubTag ("fileshared_get");

  sfh = fileSharedOpen (FN, FILESH_OPEN_READ_WRITE, FILESH_FLUSH);
  fileSharedWrite (sfh, DATAA, strlen (DATAA));
  fileSharedWrite (sfh, DATAB, strlen (DATAB));
  fileSharedWrite (sfh, DATAC, strlen (DATAC));
  fileSharedClose (sfh);

  sfh = fileSharedOpen (FN, FILESH_OPEN_READ_WRITE, FILESH_FLUSH);
  p = fileSharedGet (sfh, tbuff, sizeof (tbuff));
  ck_assert_ptr_nonnull (p);
  ck_assert_mem_eq (tbuff, DATAA, strlen (DATAA));
  ck_assert_str_eq (tbuff, DATAA);
  p = fileSharedGet (sfh, tbuff, sizeof (tbuff));
  ck_assert_ptr_nonnull (p);
  ck_assert_mem_eq (tbuff, DATAB, strlen (DATAB));
  ck_assert_str_eq (tbuff, DATAB);
  p = fileSharedGet (sfh, tbuff, sizeof (tbuff));
  ck_assert_ptr_nonnull (p);
  ck_assert_mem_eq (p, DATAC, strlen (DATAC));
  ck_assert_str_eq (tbuff, DATAC);
  p = fileSharedGet (sfh, tbuff, sizeof (tbuff));
  ck_assert_str_eq (p, "");
  fileSharedClose (sfh);

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
  tcase_add_test (tc, fileshared_open_read_write);
  tcase_add_test (tc, fileshared_open_read);
  tcase_add_test (tc, fileshared_write_trunc);
  tcase_add_test (tc, fileshared_write_append);
  tcase_add_test (tc, fileshared_write_multiple);
  tcase_add_test (tc, fileshared_write_shared);
  tcase_add_test (tc, fileshared_read);
  tcase_add_test (tc, fileshared_seek);
  tcase_add_test (tc, fileshared_read_write);
  tcase_add_test (tc, fileshared_get);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
