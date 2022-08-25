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

#include "bdj4.h"
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

/* update the fnlist in fileop/filemanip/dirop/dirlist also */
static char *fnlist [] = {
  "tmp/abc-def",
  "tmp/ÜÄÑÖ",
  "tmp/I Am the Best_내가 제일 잘 나가",
  "tmp/ははは",
  "tmp/夕陽伴我歸",
  "tmp/Ne_Русский_Шторм",
};
enum {
  fnlistsz = sizeof (fnlist) / sizeof (char *),
};

START_TEST(filemanip_move_u)
{
  FILE      *fh;
  int       rc;
  ssize_t   osz;
  ssize_t   nsz;

  for (int i = 0; i < fnlistsz; ++i) {
    char  *ofn;
    char  nfn [MAXPATHLEN];

    ofn = fnlist [i];
    snprintf (nfn, sizeof (nfn), "%s.new", ofn);
    fileopDelete (ofn);
    fileopDelete (nfn);

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    fclose (fh);

    rc = fileopFileExists (ofn);
    ck_assert_int_eq (rc, 1);
    osz = fileopSize (ofn);
    rc = filemanipMove (ofn, nfn);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (nfn);
    ck_assert_int_eq (rc, 1);
    nsz = fileopSize (nfn);
    ck_assert_int_eq (osz, nsz);

    fileopDelete (ofn);
    fileopDelete (nfn);
  }
}
END_TEST

START_TEST(filemanip_copy_u)
{
  FILE      *fh;
  int       rc;
  ssize_t   osz;
  ssize_t   nsz;
  char      *nullfn = "tmp/not-exist.txt";

  for (int i = 0; i < fnlistsz; ++i) {
    char  *ofn;
    char  nfn [MAXPATHLEN];

    ofn = fnlist [i];
    snprintf (nfn, sizeof (nfn), "%s.new", ofn);
    fileopDelete (ofn);
    fileopDelete (nfn);

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "x\n");
    fclose (fh);

    rc = fileopFileExists (ofn);
    ck_assert_int_eq (rc, 1);
    osz = fileopSize (ofn);
    rc = filemanipCopy (ofn, nfn);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn);
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (nfn);
    ck_assert_int_eq (rc, 1);
    nsz = fileopSize (nfn);
    ck_assert_int_eq (osz, nsz);

    fileopDelete (nfn);
    rc = filemanipCopy (nullfn, nfn);
    ck_assert_int_ne (rc, 0);
    rc = fileopFileExists (nfn);
    ck_assert_int_eq (rc, 0);

    fileopDelete (ofn);
    fileopDelete (nfn);
  }
}
END_TEST

START_TEST(filemanip_backup_u)
{
  FILE      *fh;
  int       rc;
  char      buff [10];

  for (int i = 0; i < fnlistsz; ++i) {
    char  ofn0a [MAXPATHLEN];
    char  ofn0 [MAXPATHLEN];
    char  ofn1 [MAXPATHLEN];
    char  ofn2 [MAXPATHLEN];
    char  ofn3 [MAXPATHLEN];

    char *ofn = fnlist [i];
    snprintf (ofn0a, sizeof (ofn0a), "%s.0", ofn);
    snprintf (ofn0, sizeof (ofn0), "%s.bak.0", ofn);
    snprintf (ofn1, sizeof (ofn1), "%s.bak.1", ofn);
    snprintf (ofn2, sizeof (ofn2), "%s.bak.2", ofn);
    snprintf (ofn3, sizeof (ofn3), "%s.bak.3", ofn);
    fileopDelete (ofn);
    fileopDelete (ofn0a);
    fileopDelete (ofn0);
    fileopDelete (ofn1);
    fileopDelete (ofn2);
    fileopDelete (ofn3);

    fh = fileopOpen (ofn, "w");
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

    fh = fileopOpen (ofn, "r");
    ck_assert_ptr_nonnull (fh);
    fgets (buff, 2, fh);
    fclose (fh);
    ck_assert_str_eq (buff, "1");

    fh = fileopOpen (ofn1, "r");
    ck_assert_ptr_nonnull (fh);
    fgets (buff, 2, fh);
    fclose (fh);
    ck_assert_str_eq (buff, "1");

    fh = fileopOpen (ofn, "w");
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

    fh = fileopOpen (ofn, "r");
    ck_assert_ptr_nonnull (fh);
    fgets (buff, 2, fh);
    fclose (fh);
    ck_assert_str_eq (buff, "2");

    fh = fileopOpen (ofn1, "r");
    ck_assert_ptr_nonnull (fh);
    fgets (buff, 2, fh);
    fclose (fh);
    ck_assert_str_eq (buff, "2");

    fh = fileopOpen (ofn2, "r");
    ck_assert_ptr_nonnull (fh);
    fgets (buff, 2, fh);
    fclose (fh);
    ck_assert_str_eq (buff, "1");

    fh = fileopOpen (ofn, "w");
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

    fh = fileopOpen (ofn, "r");
    ck_assert_ptr_nonnull (fh);
    fgets (buff, 2, fh);
    fclose (fh);
    ck_assert_str_eq (buff, "3");

    fh = fileopOpen (ofn1, "r");
    ck_assert_ptr_nonnull (fh);
    fgets (buff, 2, fh);
    fclose (fh);
    ck_assert_str_eq (buff, "3");

    fh = fileopOpen (ofn2, "r");
    ck_assert_ptr_nonnull (fh);
    fgets (buff, 2, fh);
    fclose (fh);
    ck_assert_str_eq (buff, "2");

    fileopDelete (ofn);
    fileopDelete (ofn0a);
    fileopDelete (ofn0);
    fileopDelete (ofn1);
    fileopDelete (ofn2);
    fileopDelete (ofn3);
  }
}
END_TEST

Suite *
filemanip_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("filemanip");
  tc = tcase_create ("filemanip");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, filemanip_move_a);
  tcase_add_test (tc, filemanip_copy_a);
  tcase_add_test (tc, filemanip_backup_a);
  tcase_add_test (tc, filemanip_renameall_a);
  tcase_add_test (tc, filemanip_move_u);
  tcase_add_test (tc, filemanip_copy_u);
  tcase_add_test (tc, filemanip_backup_u);
  suite_add_tcase (s, tc);
  return s;
}

