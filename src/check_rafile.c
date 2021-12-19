#include "bdjconfig.h"

#include <stdio.h>
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
  ck_assert (rafile != NULL);
  ck_assert (rafile->version == 10);
  ck_assert (rafile->size == RAFILE_REC_SIZE);
  ck_assert (rafile->count == 0L);
  raClose (rafile);
  rc = stat (RAFN, &statbuf);
  ck_assert (rc == 0 && statbuf.st_size >= 59L);
}
END_TEST

START_TEST(rafile_reopen)
{
  rafile_t      *rafile;

  rafile = raOpen (RAFN, 10);
  ck_assert (rafile != NULL);
  ck_assert (rafile->version == 10);
  ck_assert (rafile->size == RAFILE_REC_SIZE);
  ck_assert (rafile->count == 0L);
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
  ck_assert (rafile != NULL);
  ck_assert (rafile->version == 10);
  ck_assert (rafile->size == RAFILE_REC_SIZE);
  ck_assert (rafile->count == 0L);
  rc = stat (RAFN, &statbuf);
  ck_assert (rc == 0);
  lastsize = statbuf.st_size;

  raWrite (rafile, RAFILE_NEW, "aaaa");
  rc = stat (RAFN, &statbuf);
  ck_assert (rc == 0 && statbuf.st_size > lastsize);
  lastsize = statbuf.st_size;
  raWrite (rafile, RAFILE_NEW, "bbbb");
  rc = stat (RAFN, &statbuf);
  ck_assert (rc == 0 && statbuf.st_size > lastsize);
  lastsize = statbuf.st_size;
  raWrite (rafile, RAFILE_NEW, "cccc");
  rc = stat (RAFN, &statbuf);
  ck_assert (rc == 0 && statbuf.st_size > lastsize);

  ck_assert (rafile->count == 3L);
  raClose (rafile);
}
END_TEST

START_TEST(rafile_read)
{
  rafile_t      *rafile;
  char          data [RAFILE_REC_SIZE];
  size_t        rc;

  rafile = raOpen (RAFN, 10);
  ck_assert (rafile != NULL);
  ck_assert (rafile->count == 3L);
  rc = raRead (rafile, 1L, data);
  ck_assert (rc == 1);
  ck_assert (strcmp (data, "aaaa") == 0);
  rc = raRead (rafile, 2L, data);
  ck_assert (rc == 1);
  ck_assert (strcmp (data, "bbbb") == 0);
  rc = raRead (rafile, 3L, data);
  ck_assert (rc == 1);
  ck_assert (strcmp (data, "cccc") == 0);

  raClose (rafile);
}
END_TEST

START_TEST(rafile_rewrite)
{
  rafile_t      *rafile;
  struct stat   statbuf;
  int           rc;
  off_t         lastsize;

  rafile = raOpen (RAFN, 10);
  ck_assert (rafile != NULL);
  ck_assert (rafile->version == 10);
  ck_assert (rafile->count == 3L);
  rc = stat (RAFN, &statbuf);
  ck_assert (rc == 0);
  lastsize = statbuf.st_size;

  raWrite (rafile, 3L, "dddd");
  rc = stat (RAFN, &statbuf);
  ck_assert (rc == 0 && statbuf.st_size == lastsize);
  lastsize = statbuf.st_size;
  raWrite (rafile, 2L, "eeee");
  rc = stat (RAFN, &statbuf);
  ck_assert (rc == 0 && statbuf.st_size == lastsize);
  lastsize = statbuf.st_size;
  raWrite (rafile, 1L, "ffff");
  rc = stat (RAFN, &statbuf);
  ck_assert (rc == 0 && statbuf.st_size == lastsize);

  ck_assert (rafile->count == 3L);
  raClose (rafile);
}
END_TEST

START_TEST(rafile_reread)
{
  rafile_t      *rafile;
  char          data [RAFILE_REC_SIZE];

  rafile = raOpen (RAFN, 10);
  ck_assert (rafile != NULL);
  ck_assert (rafile->count == 3L);
  raRead (rafile, 1L, data);
  ck_assert (strcmp (data, "ffff") == 0);
  raRead (rafile, 2L, data);
  ck_assert (strcmp (data, "eeee") == 0);
  raRead (rafile, 3L, data);
  ck_assert (strcmp (data, "dddd") == 0);

  raClose (rafile);
}
END_TEST


START_TEST(rafile_bad_write_len)
{
  rafile_t      *rafile;
  int           rc;
  char          data [2 * RAFILE_REC_SIZE];

  rafile = raOpen (RAFN, 10);
  ck_assert (rafile != NULL);

  memset (data, 0, sizeof (data));
  memset (data, 'a', sizeof (data)-4);
  rc = raWrite (rafile, 0L, data);
  ck_assert (rc != 0);
  raClose (rafile);
}
END_TEST

START_TEST(rafile_clear)
{
  rafile_t      *rafile;
  int           rc;
  char          data [2 * RAFILE_REC_SIZE];

  rafile = raOpen (RAFN, 10);
  ck_assert (rafile != NULL);

  rc = raClear (rafile, 2L);
  ck_assert (rc == 0);

  raRead (rafile, 1L, data);
  ck_assert (strcmp (data, "ffff") == 0);
  raRead (rafile, 2L, data);
  ck_assert (strcmp (data, "") == 0);
  raRead (rafile, 3L, data);
  ck_assert (strcmp (data, "dddd") == 0);

  raClose (rafile);
}
END_TEST

START_TEST(rafile_bad_read)
{
  rafile_t      *rafile;
  size_t        rc;
  char          data [RAFILE_REC_SIZE];

  rafile = raOpen (RAFN, 10);
  ck_assert (rafile != NULL);

  rc = raRead (rafile, 0L, data);
  ck_assert (rc == 0);
  rc = raRead (rafile, 4L, data);
  ck_assert (rc == 0);

  raClose (rafile);
}
END_TEST

START_TEST(rafile_bad_clear)
{
  rafile_t      *rafile;
  int           rc;

  rafile = raOpen (RAFN, 10);
  ck_assert (rafile != NULL);

  rc = raClear (rafile, 0L);
  ck_assert (rc != 0);
  rc = raClear (rafile, 4L);
  ck_assert (rc != 0);

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
  TCase     *tc_rafile;

  s = suite_create ("RAFile Suite");
  tc_rafile = tcase_create ("RA-File");
  tcase_add_test (tc_rafile, rafile_create_new);
  tcase_add_test (tc_rafile, rafile_reopen);
  tcase_add_test (tc_rafile, rafile_write);
  tcase_add_test (tc_rafile, rafile_read);
  tcase_add_test (tc_rafile, rafile_rewrite);
  tcase_add_test (tc_rafile, rafile_reread);
  tcase_add_test (tc_rafile, rafile_bad_write_len);
  tcase_add_test (tc_rafile, rafile_clear);
  tcase_add_test (tc_rafile, rafile_bad_read);
  tcase_add_test (tc_rafile, rafile_bad_clear);
  tcase_add_test (tc_rafile, rafile_cleanup);
  suite_add_tcase (s, tc_rafile);
  return s;
}
