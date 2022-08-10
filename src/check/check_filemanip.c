#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "filemanip.h"
#include "fileop.h"
#include "check_bdj.h"
#include "slist.h"
#include "sysvars.h"

START_TEST(filemanip_move_a)
{
  FILE      *fh;
  int       rc;

  char *ofn = "tmp/abc.txt.c";
  char *nfn = "tmp/abc.txt.d";
  unlink (ofn);
  unlink (nfn);
  fh = fopen (ofn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = filemanipMove (ofn, nfn);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (nfn);
  ck_assert_int_eq (rc, 1);
  unlink (ofn);
  unlink (nfn);
}
END_TEST

START_TEST(filemanip_copy_a)
{
  FILE      *fh;
  int       rc;

  char *nullfn = "tmp/abc-z.txt";
  char *ofn = "tmp/abc-a.txt";
  char *nfn = "tmp/abc-b.txt";
  unlink (ofn);
  unlink (nfn);

  fh = fopen (ofn, "w");
  ck_assert_ptr_nonnull (fh);
  fprintf (fh, "x\n");
  fclose (fh);

  rc = fileopFileExists (nfn);
  ck_assert_int_eq (rc, 0);

  rc = filemanipCopy (ofn, nfn);
  ck_assert_int_eq (rc, 0);

  rc = fileopFileExists (ofn);
  ck_assert_int_eq (rc, 1);
  rc = fileopFileExists (nfn);
  ck_assert_int_eq (rc, 1);

  unlink (nfn);
  rc = filemanipCopy (nullfn, nfn);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (nfn);
  ck_assert_int_eq (rc, 0);

  unlink (ofn);
  unlink (nfn);
}
END_TEST

START_TEST(filemanip_backup_a)
{
  FILE      *fh;
  int       rc;
  char      buff [10];

  char *ofn = "tmp/abc.txt";
  char *ofn0a = "tmp/abc.txt.0";
  char *ofn0 = "tmp/abc.txt.bak.0";
  char *ofn1 = "tmp/abc.txt.bak.1";
  char *ofn2 = "tmp/abc.txt.bak.2";
  char *ofn3 = "tmp/abc.txt.bak.3";
  unlink (ofn);
  unlink (ofn0a);
  unlink (ofn0);
  unlink (ofn1);
  unlink (ofn2);
  unlink (ofn3);

  fh = fopen (ofn, "w");
  ck_assert_ptr_nonnull (fh);
  fprintf (fh, "1\n");
  fclose (fh);

  rc = fileopFileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn1);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn2);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn3);
  ck_assert_int_eq (rc, 0);

  filemanipBackup (ofn, 2);

  rc = fileopFileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn2);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn3);
  ck_assert_int_eq (rc, 0);

  fh = fopen (ofn, "r");
  ck_assert_ptr_nonnull (fh);
  fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "1");

  fh = fopen (ofn1, "r");
  ck_assert_ptr_nonnull (fh);
  fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "1");

  fh = fopen (ofn, "w");
  ck_assert_ptr_nonnull (fh);
  fprintf (fh, "2\n");
  fclose (fh);

  filemanipBackup (ofn, 2);

  rc = fileopFileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn2);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn3);
  ck_assert_int_eq (rc, 0);

  fh = fopen (ofn, "r");
  ck_assert_ptr_nonnull (fh);
  fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "2");

  fh = fopen (ofn1, "r");
  ck_assert_ptr_nonnull (fh);
  fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "2");

  fh = fopen (ofn2, "r");
  ck_assert_ptr_nonnull (fh);
  fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "1");

  fh = fopen (ofn, "w");
  ck_assert_ptr_nonnull (fh);
  fprintf (fh, "3\n");
  fclose (fh);

  filemanipBackup (ofn, 2);

  rc = fileopFileExists (ofn);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn0a);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn0);
  ck_assert_int_eq (rc, 0);
  rc = fileopFileExists (ofn1);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn2);
  ck_assert_int_ne (rc, 0);
  rc = fileopFileExists (ofn3);
  ck_assert_int_eq (rc, 0);

  fh = fopen (ofn, "r");
  ck_assert_ptr_nonnull (fh);
  fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "3");

  fh = fopen (ofn1, "r");
  ck_assert_ptr_nonnull (fh);
  fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "3");

  fh = fopen (ofn2, "r");
  ck_assert_ptr_nonnull (fh);
  fgets (buff, 2, fh);
  fclose (fh);
  ck_assert_str_eq (buff, "2");

  unlink (ofn);
  unlink (ofn0a);
  unlink (ofn0);
  unlink (ofn1);
  unlink (ofn2);
}
END_TEST

START_TEST(filemanip_renameall_a)
{
}

/* update the fnlist in check_fileop.c and check_dirop.c also */
static char *fnlist [] = {
  "tmp/ÜÄÑÖ",
  "tmp/I Am the Best (내가 제일 잘 나가)",
  "tmp/ははは",
  "tmp/夕陽伴我歸",
  "tmp/Ne_Русский_Шторм",
};
enum {
  fnlistsz = sizeof (fnlist) / sizeof (char *),
};

Suite *
filemanip_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("filemanip Suite");
  tc = tcase_create ("filemanip");
  tcase_add_test (tc, filemanip_move_a);
  tcase_add_test (tc, filemanip_copy_a);
  tcase_add_test (tc, filemanip_backup_a);
  tcase_add_test (tc, filemanip_renameall_a);
  suite_add_tcase (s, tc);
  return s;
}

