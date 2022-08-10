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

#include "dirlist.h"
#include "dirop.h"
#include "filemanip.h"
#include "fileop.h"
#include "check_bdj.h"
#include "slist.h"
#include "sysvars.h"

START_TEST(dirop_mkdir_isdir_a)
{
  int       rc;

  char *fn = "tmp/def";
  unlink (fn);
  rmdir (fn);
  fn = "tmp/abc/def";
  rc = diropMakeDir (fn);
  ck_assert_int_eq (rc, 0);
  fn = "tmp/abc/xyz";
  rc = diropMakeDir (fn);
  ck_assert_int_eq (rc, 0);

  rc = fileopIsDirectory ("tmp/def");
  ck_assert_int_eq (rc, 0);

  rc = fileopFileExists ("tmp/def");
  ck_assert_int_eq (rc, 0);

  rc = fileopIsDirectory ("tmp/abc");
  ck_assert_int_eq (rc, 1);

  rc = fileopFileExists ("tmp/abc");
  ck_assert_int_eq (rc, 0);

  rc = fileopIsDirectory (fn);
  ck_assert_int_eq (rc, 1);
  /* exists will return false on a directory */
  rc = fileopFileExists ("fn");
  ck_assert_int_eq (rc, 0);

  rc = fileopIsDirectory ("tmp/abc/xyz");
  ck_assert_int_eq (rc, 1);

  rmdir (fn);
  rmdir ("tmp/abc/xyz");
  rmdir ("tmp/abc");
}
END_TEST

START_TEST(dirop_del_dir_a)
{
  FILE      *fh;
  int       rc;

  /* note that this fails on windows running under msys2 */
  char *dfn = "tmp/abc";
  char *ofn = "tmp/abc/def";
  char *nfn = "tmp/abc/def/xyz.txt";
  rc = diropMakeDir (dfn);
  ck_assert_int_eq (rc, 0);
  rc = diropMakeDir (ofn);
  ck_assert_int_eq (rc, 0);
  fh = fopen (nfn, "w");
  ck_assert_ptr_nonnull (fh);
  fclose (fh);
  rc = fileopIsDirectory (dfn);
  ck_assert_int_eq (rc, 1);
  rc = fileopIsDirectory (ofn);
  ck_assert_int_eq (rc, 1);
  rc = fileopFileExists (nfn);
  ck_assert_int_eq (rc, 1);

  diropDeleteDir (dfn);
  rc = fileopFileExists (nfn);
  ck_assert_int_eq (rc, 0);
  rc = fileopIsDirectory (ofn);
  ck_assert_int_eq (rc, 0);
  rc = fileopIsDirectory (dfn);
  ck_assert_int_eq (rc, 0);
}
END_TEST

/* update the fnlist in check_fileop.c and check_filemanip.c also */
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

START_TEST(dirop_mk_is_del_u)
{
  int     rc;

  for (int i = 0; i < fnlistsz; ++i) {
    char *fn = fnlist [i];
    rc = diropMakeDir (fn);
    ck_assert_int_eq (rc, 0);
    rc = fileopFileExists (fn);
    ck_assert_int_eq (rc, 0);
    rc = fileopIsDirectory (fn);
    ck_assert_int_eq (rc, 1);
    diropDeleteDir (fn);
    rc = fileopIsDirectory (fn);
    ck_assert_int_eq (rc, 0);
  }
}

Suite *
dirop_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("dirop Suite");
  tc = tcase_create ("dirop");
  tcase_add_test (tc, dirop_mkdir_isdir_a);
  tcase_add_test (tc, dirop_del_dir_a);
  tcase_add_test (tc, dirop_mk_is_del_u);
  suite_add_tcase (s, tc);
  return s;
}

