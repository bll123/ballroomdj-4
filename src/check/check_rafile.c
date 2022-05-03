#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <check.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "rafile.h"
#include "check_bdj.h"

#define RAFN "tmp/test_rafile.dat"

START_TEST(rafile_create_new)
{
  rafile_t      *rafile;
  struct stat   statbuf;
  int           rc;

  unlink (RAFN);
  rafile = raOpen (RAFN, 10);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetVersion (rafile), 10);
  ck_assert_int_eq (raGetSize (rafile), RAFILE_REC_SIZE);
  ck_assert_int_eq (raGetCount (rafile), 0L);
  raClose (rafile);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ge (statbuf.st_size, 59L);
}
END_TEST

START_TEST(rafile_reopen)
{
  rafile_t      *rafile;

  rafile = raOpen (RAFN, 10);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetVersion (rafile), 10);
  ck_assert_int_eq (raGetSize (rafile), RAFILE_REC_SIZE);
  ck_assert_int_eq (raGetCount (rafile), 0L);
  raClose (rafile);
}
END_TEST

START_TEST(rafile_write)
{
  rafile_t      *rafile;
  struct stat   statbuf;
  int           rc;
  off_t         lastsize;

  rafile = raOpen (RAFN, 10);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetVersion (rafile), 10);
  ck_assert_int_eq (raGetSize (rafile), RAFILE_REC_SIZE);
  ck_assert_int_eq (raGetCount (rafile), 0L);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  lastsize = statbuf.st_size;

  raWrite (rafile, RAFILE_NEW, "aaaa");
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ge (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;
  ck_assert_int_eq (raGetCount (rafile), 1L);
  raWrite (rafile, RAFILE_NEW, "bbbb");
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ge (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;
  ck_assert_int_eq (raGetCount (rafile), 2L);
  raWrite (rafile, RAFILE_NEW, "cccc");
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_ge (statbuf.st_size, lastsize);
  ck_assert_int_eq (raGetCount (rafile), 3L);

  raClose (rafile);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  /* rawrite writes a single byte at the end so that the last record */
  /* has a size */
  ck_assert_int_eq (statbuf.st_size, RRN_TO_OFFSET(4L) + 1);
}
END_TEST

START_TEST(rafile_read)
{
  rafile_t      *rafile;
  char          data [RAFILE_REC_SIZE];
  ssize_t        rc;

  rafile = raOpen (RAFN, 10);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetCount (rafile), 3L);
  rc = raRead (rafile, 1L, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "aaaa");
  rc = raRead (rafile, 2L, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "bbbb");
  rc = raRead (rafile, 3L, data);
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

  rafile = raOpen (RAFN, 10);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetVersion (rafile), 10);
  ck_assert_int_eq (raGetCount (rafile), 3L);
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  lastsize = statbuf.st_size;

  raWrite (rafile, 3L, "dddd");
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_eq (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;
  raWrite (rafile, 2L, "eeee");
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_eq (statbuf.st_size, lastsize);
  lastsize = statbuf.st_size;
  raWrite (rafile, 1L, "ffff");
  rc = stat (RAFN, &statbuf);
  ck_assert_int_eq (rc, 0);
  ck_assert_int_eq (statbuf.st_size, lastsize);

  ck_assert_int_eq (raGetCount (rafile), 3L);
  raClose (rafile);
}
END_TEST

START_TEST(rafile_reread)
{
  rafile_t      *rafile;
  char          data [RAFILE_REC_SIZE];
  ssize_t       rc;

  rafile = raOpen (RAFN, 10);
  ck_assert_ptr_nonnull (rafile);
  ck_assert_int_eq (raGetCount (rafile), 3L);
  rc = raRead (rafile, 1L, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "ffff");
  rc = raRead (rafile, 2L, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "eeee");
  rc = raRead (rafile, 3L, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "dddd");

  raClose (rafile);
}
END_TEST


START_TEST(rafile_bad_write_len)
{
  rafile_t      *rafile;
  ssize_t       rc;
  char          data [2 * RAFILE_REC_SIZE];

  rafile = raOpen (RAFN, 10);
  ck_assert_ptr_nonnull (rafile);

  memset (data, 0, sizeof (data));
  memset (data, 'a', sizeof (data)-4);
  rc = raWrite (rafile, 0L, data);
  ck_assert_int_ne (rc, 0);
  raClose (rafile);
}
END_TEST

START_TEST(rafile_clear)
{
  rafile_t      *rafile;
  ssize_t       rc;
  char          data [2 * RAFILE_REC_SIZE];

  rafile = raOpen (RAFN, 10);
  ck_assert_ptr_nonnull (rafile);

  rc = raClear (rafile, 2L);
  ck_assert_int_eq (rc, 0);

  rc = raRead (rafile, 1L, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "ffff");
  rc = raRead (rafile, 2L, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "");
  rc = raRead (rafile, 3L, data);
  ck_assert_int_eq (rc, 1);
  ck_assert_str_eq (data, "dddd");

  raClose (rafile);
}
END_TEST

START_TEST(rafile_bad_read)
{
  rafile_t      *rafile;
  ssize_t        rc;
  char          data [RAFILE_REC_SIZE];

  rafile = raOpen (RAFN, 10);
  ck_assert_ptr_nonnull (rafile);

  rc = raRead (rafile, 0L, data);
  ck_assert_int_eq (rc, 0);
  rc = raRead (rafile, 4L, data);
  ck_assert_int_eq (rc, 0);

  raClose (rafile);
}
END_TEST

START_TEST(rafile_bad_clear)
{
  rafile_t      *rafile;
  ssize_t       rc;

  rafile = raOpen (RAFN, 10);
  ck_assert_ptr_nonnull (rafile);

  rc = raClear (rafile, 0L);
  ck_assert_int_ne (rc, 0);
  rc = raClear (rafile, 4L);
  ck_assert_int_ne (rc, 0);

  raClose (rafile);
}
END_TEST

START_TEST(rafile_cleanup)
{
  unlink (RAFN);
}

Suite *
rafile_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("RAFile Suite");
  tc = tcase_create ("RA-File");
  tcase_add_test (tc, rafile_create_new);
  tcase_add_test (tc, rafile_reopen);
  tcase_add_test (tc, rafile_write);
  tcase_add_test (tc, rafile_read);
  tcase_add_test (tc, rafile_rewrite);
  tcase_add_test (tc, rafile_reread);
  tcase_add_test (tc, rafile_bad_write_len);
  tcase_add_test (tc, rafile_clear);
  tcase_add_test (tc, rafile_bad_read);
  tcase_add_test (tc, rafile_bad_clear);
  tcase_add_test (tc, rafile_cleanup);
  suite_add_tcase (s, tc);
  return s;
}
