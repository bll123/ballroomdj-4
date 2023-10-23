/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdj4.h"
#include "bdjmsg.h"
#include "bdjstring.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "lock.h"
#include "log.h"
#include "pathbld.h"
#include "sysvars.h"

#define LOCK_FN "test_lock"
#define BAD_FULL_LOCK_FN "tmpz/bad_lock.lck"

static char fulllockfn [MAXPATHLEN];

static void
setup (void)
{
  pathbldMakePath (fulllockfn, sizeof (fulllockfn),
      LOCK_FN, BDJ4_LOCK_EXT, PATHBLD_MP_DIR_LOCK);

  bdjvarsInit ();
}

static void
teardown (void)
{
  bdjvarsCleanup ();
}

START_TEST(lock_name)
{
  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- lock_name");
  mdebugSubTag ("lock_name");

  ck_assert_str_eq (lockName (ROUTE_PLAYERUI), "playerui");
}
END_TEST

START_TEST(lock_acquire_release)
{
  int           rc;
  struct stat   statbuf;
  FILE          *fh;
  pid_t         pid;
  pid_t         fpid;
  int64_t       temp;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- lock_acquire_release");
  mdebugSubTag ("lock_acquire_release");

  pid = getpid ();
  unlink (fulllockfn);
  rc = lockAcquire (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_gt (rc, 0);
  rc = stat (fulllockfn, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_gt (statbuf.st_size, 0);
  fh = fopen (fulllockfn, "r");
  rc = fscanf (fh, "%" PRId64, &temp);
  fpid = (pid_t) temp;
  fclose (fh);
  ck_assert_int_eq (fpid, pid);
  rc = lockRelease (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_eq (rc, 0);
  rc = stat (fulllockfn, &statbuf);
  ck_assert_int_lt (rc, 0);
  unlink (fulllockfn);
}
END_TEST

START_TEST(lock_acquire_no_dir)
{
  int           rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- lock_acquire_no_dir");
  mdebugSubTag ("lock_acquire_no_dir");

  unlink (BAD_FULL_LOCK_FN);
  rc = lockAcquire (BAD_FULL_LOCK_FN, PATHBLD_LOCK_FFN);
  ck_assert_int_lt (rc, 0);
}
END_TEST

START_TEST(lock_already)
{
  int           rc;
  struct stat   statbuf;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- lock_already");
  mdebugSubTag ("lock_already");

  unlink (fulllockfn);
  rc = lockAcquire (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_gt (rc, 0);
  rc = stat (fulllockfn, &statbuf);
  ck_assert_int_eq (rc, 0);
  rc = lockAcquire (LOCK_FN, PATHBLD_MP_NONE | LOCK_TEST_SKIP_SELF);
  ck_assert_int_lt (rc, 0);
  rc = lockRelease (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_eq (rc, 0);
  rc = stat (fulllockfn, &statbuf);
  ck_assert_int_lt (rc, 0);
  unlink (fulllockfn);
}
END_TEST

START_TEST(lock_other_dead)
{
  int           rc;
  struct stat   statbuf;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- lock_other_dead");
  mdebugSubTag ("lock_other_dead");

  unlink (fulllockfn);
  rc = lockAcquire (LOCK_FN, PATHBLD_MP_NONE | LOCK_TEST_OTHER_PID);
  ck_assert_int_gt (rc, 0);
  rc = stat (fulllockfn, &statbuf);
  ck_assert_int_eq (rc, 0);
  rc = lockAcquire (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_gt (rc, 0);
  rc = stat (fulllockfn, &statbuf);
  ck_assert_int_eq (rc, 0);
  rc = lockRelease (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_eq (rc, 0);
  rc = stat (fulllockfn, &statbuf);
  ck_assert_int_lt (rc, 0);
  unlink (fulllockfn);
}
END_TEST

START_TEST(lock_unlock_fail)
{
  int           rc;
  struct stat   statbuf;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- lock_unlock_fail");
  mdebugSubTag ("lock_unlock_fail");

  unlink (fulllockfn);
  rc = lockAcquire (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_gt (rc, 0);
  rc = stat (fulllockfn, &statbuf);
  ck_assert_int_eq (rc, 0);
  rc = lockRelease (LOCK_FN, PATHBLD_MP_NONE | LOCK_TEST_OTHER_PID);
  ck_assert_int_lt (rc, 0);
  rc = lockRelease (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_eq (rc, 0);
  rc = lockRelease (BAD_FULL_LOCK_FN, PATHBLD_LOCK_FFN);
  ck_assert_int_lt (rc, 0);
  rc = stat (fulllockfn, &statbuf);
  ck_assert_int_lt (rc, 0);
  unlink (fulllockfn);
}
END_TEST

START_TEST(lock_exists)
{
  int           rc;
  pid_t         pid;
  pid_t         tpid;
  pid_t         fpid;
  int64_t       temp;
  FILE          *fh;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- lock_exists");
  mdebugSubTag ("lock_exists");

  pid = getpid ();
  unlink (fulllockfn);
  rc = lockAcquire (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_gt (rc, 0);

  fh = fopen (fulllockfn, "r");
  rc = fscanf (fh, "%" PRId64, &temp);
  fpid = (pid_t) temp;
  fclose (fh);
  ck_assert_int_eq (fpid, pid);

  /* lock file exists, same process */
  tpid = lockExists (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_eq (tpid, 0);

  /* lock file exists, same process */
  tpid = lockExists (LOCK_FN, PATHBLD_MP_NONE | LOCK_TEST_SKIP_SELF);
  ck_assert_int_ne (tpid, 0);

  rc = lockRelease (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_eq (rc, 0);
  /* lock file does not exist */
  tpid = lockExists (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_eq (tpid, 0);

  fh = fopen (fulllockfn, "w");
  temp = 94534;
  fprintf (fh, "%" PRId64, temp);
  fclose (fh);
  /* lock file exists, no associated process */
  tpid = lockExists (LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_eq (tpid, 0);
  unlink (fulllockfn);
}
END_TEST

Suite *
lock_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("lock");
  tc = tcase_create ("lock-base");
  tcase_set_tags (tc, "libbasic");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, lock_name);
  tcase_add_test (tc, lock_acquire_release);
  tcase_add_test (tc, lock_acquire_no_dir);
  suite_add_tcase (s, tc);
  tc = tcase_create ("lock-already");
  tcase_set_tags (tc, "libbasic");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, lock_already);
  suite_add_tcase (s, tc);
  tc = tcase_create ("lock-more");
  tcase_set_tags (tc, "libbasic");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, lock_other_dead);
  tcase_add_test (tc, lock_unlock_fail);
  tcase_add_test (tc, lock_exists);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
