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

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdj4.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "filemanip.h"
#include "fileop.h"
#include "log.h"
#include "slist.h"
#include "sysvars.h"

/* update the fnlist in fileop/filemanip/dirop/dirlist also */
static char *fnlist [] = {
  "tmp/abc-def.txt",
  "tmp/ÜÄÑÖ.txt",
  "tmp/I Am the Best_내가 제일 잘 나가.txt",
  "tmp/ははは.txt",
  "tmp/夕陽伴我歸.txt",
  "tmp/Ne_Русский_Шторм.txt",
};
static char *nfnlist [] = {
  "tmp/abc-def-new.txt",
  "tmp/ÜÄÑÖ-new.txt",
  "tmp/I Am the Best_내가 제일 잘 나가-new.txt",
  "tmp/ははは-new.txt",
  "tmp/夕陽伴我歸-new.txt",
  "tmp/Ne_Русский_Шторм-new.txt",
};
enum {
  fnlistsz = sizeof (fnlist) / sizeof (char *),
};


START_TEST(filemanip_move)
{
  FILE      *fh;
  int       rc;
  ssize_t   osz;
  ssize_t   nsz;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- filemanip_move");
  mdebugSubTag ("filemanip_move");

  for (int i = 0; i < fnlistsz; ++i) {
    char  *ofn;
    char  nfn [BDJ4_PATH_MAX];

    ofn = fnlist [i];
    snprintf (nfn, sizeof (nfn), "%s.new", ofn);
    fileopDelete (ofn);
    fileopDelete (nfn);

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    mdextfclose (fh);
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

START_TEST(filemanip_copy)
{
  FILE      *fh;
  int       rc;
  ssize_t   osz;
  ssize_t   nsz;
  char      *nullfn = "tmp/not-exist.txt";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- filemanip_copy");
  mdebugSubTag ("filemanip_copy");

  for (int i = 0; i < fnlistsz; ++i) {
    char  *ofn;
    char  nfn [BDJ4_PATH_MAX];

    ofn = fnlist [i];
    snprintf (nfn, sizeof (nfn), "%s.new", ofn);
    fileopDelete (ofn);
    fileopDelete (nfn);

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "x\n");
    mdextfclose (fh);
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

START_TEST(filemanip_backup)
{
  FILE      *fh;
  int       rc;
  char      buff [10];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- filemanip_backup");
  mdebugSubTag ("filemanip_backup");

  for (int i = 0; i < fnlistsz; ++i) {
    char  ofn0a [BDJ4_PATH_MAX];
    char  ofn0 [BDJ4_PATH_MAX];
    char  ofn1 [BDJ4_PATH_MAX];
    char  ofn2 [BDJ4_PATH_MAX];
    char  ofn3 [BDJ4_PATH_MAX];

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
    mdextfclose (fh);
    fclose (fh);

    rc = fileopFileExists (ofn);
    ck_assert_int_eq (rc, 1);
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
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (ofn0a);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn0);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn1);
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (ofn2);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn3);
    ck_assert_int_eq (rc, 0);

    fh = fileopOpen (ofn, "r");
    ck_assert_ptr_nonnull (fh);
    (void) ! fgets (buff, 2, fh);
    mdextfclose (fh);
    fclose (fh);
    ck_assert_str_eq (buff, "1");

    fh = fileopOpen (ofn1, "r");
    ck_assert_ptr_nonnull (fh);
    (void) ! fgets (buff, 2, fh);
    mdextfclose (fh);
    fclose (fh);
    ck_assert_str_eq (buff, "1");

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "2\n");
    mdextfclose (fh);
    fclose (fh);

    filemanipBackup (ofn, 2);

    rc = fileopFileExists (ofn);
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (ofn0a);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn0);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn1);
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (ofn2);
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (ofn3);
    ck_assert_int_eq (rc, 0);

    fh = fileopOpen (ofn, "r");
    ck_assert_ptr_nonnull (fh);
    (void) ! fgets (buff, 2, fh);
    mdextfclose (fh);
    fclose (fh);
    ck_assert_str_eq (buff, "2");

    fh = fileopOpen (ofn1, "r");
    ck_assert_ptr_nonnull (fh);
    (void) ! fgets (buff, 2, fh);
    mdextfclose (fh);
    fclose (fh);
    ck_assert_str_eq (buff, "2");

    fh = fileopOpen (ofn2, "r");
    ck_assert_ptr_nonnull (fh);
    (void) ! fgets (buff, 2, fh);
    mdextfclose (fh);
    fclose (fh);
    ck_assert_str_eq (buff, "1");

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "3\n");
    mdextfclose (fh);
    fclose (fh);

    filemanipBackup (ofn, 2);

    rc = fileopFileExists (ofn);
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (ofn0a);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn0);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn1);
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (ofn2);
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (ofn3);
    ck_assert_int_eq (rc, 0);

    fh = fileopOpen (ofn, "r");
    ck_assert_ptr_nonnull (fh);
    (void) ! fgets (buff, 2, fh);
    mdextfclose (fh);
    fclose (fh);
    ck_assert_str_eq (buff, "3");

    fh = fileopOpen (ofn1, "r");
    ck_assert_ptr_nonnull (fh);
    (void) ! fgets (buff, 2, fh);
    mdextfclose (fh);
    fclose (fh);
    ck_assert_str_eq (buff, "3");

    fh = fileopOpen (ofn2, "r");
    ck_assert_ptr_nonnull (fh);
    (void) ! fgets (buff, 2, fh);
    mdextfclose (fh);
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

START_TEST(filemanip_renameall)
{
  FILE      *fh;
  int       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- filemanip_renameall");
  mdebugSubTag ("filemanip_renameall");

  for (int i = 0; i < fnlistsz; ++i) {
    char  ofn0 [BDJ4_PATH_MAX];
    char  ofn1 [BDJ4_PATH_MAX];
    char  ofn2 [BDJ4_PATH_MAX];
    char  ofn3 [BDJ4_PATH_MAX];
    char  nfn0 [BDJ4_PATH_MAX];
    char  nfn1 [BDJ4_PATH_MAX];
    char  nfn2 [BDJ4_PATH_MAX];
    char  nfn3 [BDJ4_PATH_MAX];

    char *ofn = fnlist [i];
    char *nfn = nfnlist [i];

    snprintf (ofn0, sizeof (ofn0), "%s.bak.0", ofn);
    snprintf (ofn1, sizeof (ofn1), "%s.bak.1", ofn);
    snprintf (ofn2, sizeof (ofn2), "%s.bak.2", ofn);
    snprintf (ofn3, sizeof (ofn3), "%s.bak.3", ofn);
    snprintf (nfn0, sizeof (nfn0), "%s.bak.0", nfn);
    snprintf (nfn1, sizeof (nfn1), "%s.bak.1", nfn);
    snprintf (nfn2, sizeof (nfn2), "%s.bak.2", nfn);
    snprintf (nfn3, sizeof (nfn3), "%s.bak.3", nfn);
    fileopDelete (ofn);
    fileopDelete (ofn0);
    fileopDelete (ofn1);
    fileopDelete (ofn2);
    fileopDelete (ofn3);
    fileopDelete (nfn);
    fileopDelete (nfn0);
    fileopDelete (nfn1);
    fileopDelete (nfn2);
    fileopDelete (nfn3);

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "1\n");
    mdextfclose (fh);
    fclose (fh);

    filemanipBackup (ofn, 2);

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "2\n");
    mdextfclose (fh);
    fclose (fh);

    filemanipBackup (ofn, 2);

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "3\n");
    mdextfclose (fh);
    fclose (fh);

    filemanipBackup (ofn, 2);

    /* at this point, ofn, ofn1, and ofn2 exist */

    filemanipRenameAll (ofn, nfn);

    rc = fileopFileExists (ofn);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn0);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn1);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn2);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn3);
    ck_assert_int_eq (rc, 0);

    rc = fileopFileExists (nfn);
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (nfn0);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (nfn1);
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (nfn2);
    ck_assert_int_eq (rc, 1);
    rc = fileopFileExists (nfn3);
    ck_assert_int_eq (rc, 0);

    fileopDelete (ofn);
    fileopDelete (ofn0);
    fileopDelete (ofn1);
    fileopDelete (ofn2);
    fileopDelete (ofn3);
    fileopDelete (nfn);
    fileopDelete (nfn0);
    fileopDelete (nfn1);
    fileopDelete (nfn2);
    fileopDelete (nfn3);
  }
}
END_TEST

START_TEST(filemanip_deleteall)
{
  FILE      *fh;
  int       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- filemanip_deleteall");
  mdebugSubTag ("filemanip_deleteall");

  for (int i = 0; i < fnlistsz; ++i) {
    char  ofn0 [BDJ4_PATH_MAX];
    char  ofn1 [BDJ4_PATH_MAX];
    char  ofn2 [BDJ4_PATH_MAX];
    char  ofn3 [BDJ4_PATH_MAX];

    char *ofn = fnlist [i];

    snprintf (ofn0, sizeof (ofn0), "%s.bak.0", ofn);
    snprintf (ofn1, sizeof (ofn1), "%s.bak.1", ofn);
    snprintf (ofn2, sizeof (ofn2), "%s.bak.2", ofn);
    snprintf (ofn3, sizeof (ofn3), "%s.bak.3", ofn);
    fileopDelete (ofn);
    fileopDelete (ofn0);
    fileopDelete (ofn1);
    fileopDelete (ofn2);
    fileopDelete (ofn3);

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "1\n");
    mdextfclose (fh);
    fclose (fh);

    filemanipBackup (ofn, 2);

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "2\n");
    mdextfclose (fh);
    fclose (fh);

    filemanipBackup (ofn, 2);

    fh = fileopOpen (ofn, "w");
    ck_assert_ptr_nonnull (fh);
    fprintf (fh, "3\n");
    mdextfclose (fh);
    fclose (fh);

    filemanipBackup (ofn, 2);

    /* at this point, ofn, ofn1, and ofn2 exist */

    filemanipDeleteAll (ofn);

    rc = fileopFileExists (ofn);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn0);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn1);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn2);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (ofn3);
    ck_assert_int_eq (rc, 0);

    fileopDelete (ofn);
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
  tcase_add_test (tc, filemanip_move);
  tcase_add_test (tc, filemanip_copy);
  tcase_add_test (tc, filemanip_backup);
  tcase_add_test (tc, filemanip_renameall);
  tcase_add_test (tc, filemanip_deleteall);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
