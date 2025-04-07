/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "lock.h"
#include "log.h"
#include "pathbld.h"
#include "rafile.h"
#include "check_bdj.h"
#include "mdebug.h"

#define RAFN "tmp/test_rafile.dat"

START_TEST(rafile_create_new)
{
  rafile_t      *rafile;
  struct stat   statbuf;
  int           rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_create_new");
  mdebugSubTag ("rafile_create_new");

  unlink (RAFN);
  rafile = raOpen (RAFN, 10, RAFILE_RW);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetVersion (rafile), 10);
  ck_assert_int_eq (raGetSize (rafile), RAFILE_REC_SIZE);
  ck_assert_int_eq (raGetCount (rafile), 0);
  ck_assert_int_eq (raGetNextRRN (rafile), 1);
  raClose (rafile);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ge (statbuf.st_size, 59);
}
END_TEST

START_TEST(rafile_reopen_ro)
{
  rafile_t      *rafile;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_reopen_ro");
  mdebugSubTag ("rafile_reopen_ro");

  rafile = raOpen (RAFN, 10, RAFILE_RO);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetVersion (rafile), 10);
  ck_assert_int_eq (raGetSize (rafile), RAFILE_REC_SIZE);
  ck_assert_int_eq (raGetCount (rafile), 0);
  raClose (rafile);
}
END_TEST

START_TEST(rafile_write)
{
  rafile_t      *rafile;
  struct stat   statbuf;
  int           rc;
  off_t         lastsize;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_write");
  mdebugSubTag ("rafile_write");

  rafile = raOpen (RAFN, 10, RAFILE_RW);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetVersion (rafile), 10);
  ck_assert_int_eq (raGetSize (rafile), RAFILE_REC_SIZE);
  ck_assert_int_eq (raGetCount (rafile), 0);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  lastsize = statbuf.st_size;

  ck_assert_int_eq (raGetNextRRN (rafile), 1);
  raWrite (rafile, RAFILE_NEW, "aaaa", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ge (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;
  ck_assert_int_eq (raGetCount (rafile), 1);

  ck_assert_int_eq (raGetNextRRN (rafile), 2);
  raWrite (rafile, RAFILE_NEW, "bbbb", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ge (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;
  ck_assert_int_eq (raGetCount (rafile), 2);

  ck_assert_int_eq (raGetNextRRN (rafile), 3);
  raWrite (rafile, RAFILE_NEW, "cccc", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ge (statbuf.st_size, lastsize);
  ck_assert_int_eq (raGetCount (rafile), 3);

  ck_assert_int_eq (raGetNextRRN (rafile), 4);
  raClose (rafile);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  /* rawrite writes a single byte at the end so that the last record */
  /* has a size */
  ck_assert_int_eq (statbuf.st_size, RRN_TO_OFFSET(4L) + 1);
}
END_TEST

START_TEST(rafile_write_batch)
{
  rafile_t      *rafile;
  struct stat   statbuf;
  int           rc;
  off_t         lastsize;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_write_batch");
  mdebugSubTag ("rafile_write_batch");

  rafile = raOpen (RAFN, 10, RAFILE_RW);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetCount (rafile), 3);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  lastsize = statbuf.st_size;

  raStartBatch (rafile);
  rc = lockExists (RAFILE_LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_eq (rc, 0);

  raWrite (rafile, RAFILE_NEW, "dddd", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ge (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;
  ck_assert_int_eq (raGetCount (rafile), 4);

  ck_assert_int_eq (raGetNextRRN (rafile), 5);
  raWrite (rafile, RAFILE_NEW, "eeee", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ge (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;
  ck_assert_int_eq (raGetCount (rafile), 5);

  ck_assert_int_eq (raGetNextRRN (rafile), 6);
  raWrite (rafile, RAFILE_NEW, "ffff", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ge (statbuf.st_size, lastsize);
  ck_assert_int_eq (raGetCount (rafile), 6);

  ck_assert_int_eq (raGetNextRRN (rafile), 7);
  rc = lockExists (RAFILE_LOCK_FN, PATHBLD_MP_NONE | LOCK_TEST_SKIP_SELF);
  ck_assert_int_eq (rc, getpid());
  raEndBatch (rafile);
  rc = lockExists (RAFILE_LOCK_FN, PATHBLD_MP_NONE);
  ck_assert_int_eq (rc, 0);

  raClose (rafile);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  /* rawrite writes a single byte at the end so that the last record */
  /* has a size */
  ck_assert_int_eq (statbuf.st_size, RRN_TO_OFFSET(7L) + 1);
}
END_TEST

START_TEST(rafile_read)
{
  rafile_t      *rafile;
  char          data [RAFILE_REC_SIZE];
  int           rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_read");
  mdebugSubTag ("rafile_read");

  rafile = raOpen (RAFN, 10, RAFILE_RO);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetCount (rafile), 6);
  rc = raRead (rafile, 1, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "aaaa");
  rc = raRead (rafile, 2, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "bbbb");
  rc = raRead (rafile, 3, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "cccc");

  raClose (rafile);
}
END_TEST

START_TEST(rafile_rewrite)
{
  rafile_t      *rafile;
  struct stat   statbuf;
  ssize_t       rc;
  off_t         lastsize;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_rewrite");
  mdebugSubTag ("rafile_rewrite");

  rafile = raOpen (RAFN, 10, RAFILE_RW);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetVersion (rafile), 10);
  ck_assert_int_eq (raGetCount (rafile), 6);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  lastsize = statbuf.st_size;

  raWrite (rafile, 3, "gggg", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_eq (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;

  raWrite (rafile, 2, "hhhh", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_eq (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;

  raWrite (rafile, 1, "iiii", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_eq (statbuf.st_size, lastsize);

  ck_assert_int_eq (raGetCount (rafile), 6);
  raClose (rafile);
}
END_TEST

START_TEST(rafile_reread)
{
  rafile_t      *rafile;
  char          data [RAFILE_REC_SIZE];
  ssize_t       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_reread");
  mdebugSubTag ("rafile_reread");

  rafile = raOpen (RAFN, 10, RAFILE_RO);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetCount (rafile), 6);
  rc = raRead (rafile, 1, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "iiii");
  rc = raRead (rafile, 2, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "hhhh");
  rc = raRead (rafile, 3, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "gggg");
  ck_assert_int_eq (raGetNextRRN (rafile), 7);
  raClose (rafile);
}
END_TEST

START_TEST(rafile_write_read)
{
  rafile_t      *rafile;
  struct stat   statbuf;
  ssize_t       rc;
  off_t         lastsize;
  char          data [RAFILE_REC_SIZE];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_rewrite");
  mdebugSubTag ("rafile_rewrite");

  rafile = raOpen (RAFN, 10, RAFILE_RW);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetVersion (rafile), 10);
  ck_assert_int_eq (raGetCount (rafile), 6);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  lastsize = statbuf.st_size;

  raWrite (rafile, 4, "jjjj", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_eq (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;

  rc = raRead (rafile, 4, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "jjjj");

  raWrite (rafile, 4, "mmmm", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_eq (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;

  rc = raRead (rafile, 4, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "mmmm");

  raWrite (rafile, 3, "kkkk", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_eq (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;

  rc = raRead (rafile, 3, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "kkkk");

  raWrite (rafile, 2, "llll", -1);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_eq (statbuf.st_size, lastsize);

  rc = raRead (rafile, 2, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "llll");

  rc = raRead (rafile, 1, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "iiii");

  ck_assert_int_eq (raGetNextRRN (rafile), 7);
  ck_assert_int_eq (raGetCount (rafile), 6);
  raClose (rafile);
}
END_TEST

START_TEST(rafile_bad_write_len)
{
  rafile_t      *rafile;
  ssize_t       rc;
  char          data [2 * RAFILE_REC_SIZE];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_bad_write_len");
  mdebugSubTag ("rafile_bad_write_len");

  rafile = raOpen (RAFN, 10, RAFILE_RW);
  ck_assert_ptr_nonnull (rafile);

  memset (data, 0, sizeof (data));
  memset (data, 'a', sizeof (data)-4);
  rc = raWrite (rafile, 0, data, -1);
  ck_assert_int_eq (rc, 0);
  raClose (rafile);
}
END_TEST

START_TEST(rafile_clear)
{
  rafile_t      *rafile;
  ssize_t       rc;
  char          data [2 * RAFILE_REC_SIZE];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_clear");
  mdebugSubTag ("rafile_clear");

  rafile = raOpen (RAFN, 10, RAFILE_RW);
  ck_assert_ptr_nonnull (rafile);

  rc = raClear (rafile, 2);
  ck_assert_int_eq (rc, true);

  rc = raRead (rafile, 1, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "iiii");
  rc = raRead (rafile, 2, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "");
  rc = raRead (rafile, 3, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "kkkk");

  raClose (rafile);
}
END_TEST

START_TEST(rafile_bad_read)
{
  rafile_t      *rafile;
  ssize_t        rc;
  char          data [RAFILE_REC_SIZE];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_bad_read");
  mdebugSubTag ("rafile_bad_read");

  rafile = raOpen (RAFN, 10, RAFILE_RO);
  ck_assert_ptr_nonnull (rafile);

  rc = raRead (rafile, 0, data);
  ck_assert_int_eq (rc, 0);
  rc = raRead (rafile, 9, data);
  ck_assert_int_eq (rc, 0);

  raClose (rafile);
}
END_TEST

START_TEST(rafile_bad_clear)
{
  rafile_t      *rafile;
  ssize_t       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_bad_clear");
  mdebugSubTag ("rafile_bad_clear");

  rafile = raOpen (RAFN, 10, RAFILE_RW);
  ck_assert_ptr_nonnull (rafile);

  rc = raClear (rafile, 0);
  ck_assert_int_ne (rc, true);
  rc = raClear (rafile, 9);
  ck_assert_int_ne (rc, true);

  raClose (rafile);
}
END_TEST

START_TEST(rafile_cleanup)
{
  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- rafile_cleanup");
  mdebugSubTag ("rafile_cleanup");

  unlink (RAFN);
}
END_TEST

Suite *
rafile_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("rafile");
  tc = tcase_create ("rafile");
  tcase_set_tags (tc, "libbasic");
  tcase_add_test (tc, rafile_create_new);
  tcase_add_test (tc, rafile_reopen_ro);
  tcase_add_test (tc, rafile_write);
  tcase_add_test (tc, rafile_write_batch);
  tcase_add_test (tc, rafile_read);
  tcase_add_test (tc, rafile_rewrite);
  tcase_add_test (tc, rafile_reread);
  tcase_add_test (tc, rafile_write_read);
  tcase_add_test (tc, rafile_bad_write_len);
  tcase_add_test (tc, rafile_clear);
  tcase_add_test (tc, rafile_bad_read);
  tcase_add_test (tc, rafile_bad_clear);
  tcase_add_test (tc, rafile_cleanup);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
