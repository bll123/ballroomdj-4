/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdj4.h"
#include "audiosrc.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "check_bdj.h"
#include "dirop.h"
#include "fileop.h"
#include "filemanip.h"
#include "mdebug.h"
#include "log.h"
#include "sysvars.h"

typedef struct {
  const char  *test;
  const char  *relpath;
  const char  *fullpath;
  int         type;
} chk_audsrc_t;

static chk_audsrc_t tvalues [] = {
  { "abc123", "abc123", "/testpath/abc123", AUDIOSRC_TYPE_FILE },
  { "/stuff", "/stuff", "/stuff", AUDIOSRC_TYPE_FILE },
  { "C:/there", "C:/there", "C:/there", AUDIOSRC_TYPE_FILE },
  { "d:/here", "d:/here", "d:/here", AUDIOSRC_TYPE_FILE },
  { "d:/here", "d:/here", "d:/here", AUDIOSRC_TYPE_FILE },
  { "unknown://stuff", "", "", AUDIOSRC_TYPE_NONE },
  { "bdj4://stuff", "bdj4://stuff", "bdj4://stuff", AUDIOSRC_TYPE_BDJ4 },
  { "file:///testpath/def456", "def456", "/testpath/def456", AUDIOSRC_TYPE_FILE },
};
enum {
  tvaluesz = sizeof (tvalues) / sizeof (chk_audsrc_t),
};

/* copied from the dirlist tests */

typedef struct {
  int       type;
  char      *name;
  char      *fname;
  int       flag;
} chk_as_list_t;

enum {
  CHK_DIR,
  CHK_FILE,
};

static chk_as_list_t lvalues [] = {
  { CHK_DIR,  "tmp/abc", NULL, 0 },
  { CHK_FILE, "tmp/abc/abc.txt", "abc.txt", 0 },
  { CHK_DIR,  "tmp/abc/def", NULL, 0 },
  { CHK_FILE, "tmp/abc/def/def.txt", "def.txt", 0 },
  { CHK_DIR,  "tmp/abc/ghi", "chk", 0 },
  { CHK_FILE, "tmp/abc/ghi/ghi.txt", "ghi.txt", 0 },
  { CHK_FILE, "tmp/abc/ghi/ghi.txt.original", "ghi.txt.original", 0 },
  { CHK_DIR,  "tmp/abc/ÄÑÄÑ", NULL, 0 },
  { CHK_FILE, "tmp/abc/ÄÑÄÑ/abc-def.txt", "abc-def.txt", 0 },
  { CHK_FILE, "tmp/abc/ÄÑÄÑ/ÜÄÑÖ.txt", "ÜÄÑÖ.txt", 0 },
  { CHK_DIR,  "tmp/abc/夕陽伴我歸", NULL, 0 },
  { CHK_FILE, "tmp/abc/夕陽伴我歸/내가제일잘나가.txt", "내가제일잘나가.txt", 0 },
  { CHK_FILE, "tmp/abc/夕陽伴我歸/ははは.txt", "ははは.txt", 0 },
  { CHK_FILE, "tmp/abc/夕陽伴我歸/夕陽伴我歸.txt", "夕陽伴我歸.txt", 0 },
  { CHK_FILE, "tmp/abc/夕陽伴我歸/Ne_Русский_Шторм.txt", "Ne_Русский_Шторм.txt", 0 }
};
enum {
  lvaluesz = sizeof (lvalues) / sizeof (chk_as_list_t),
};
static int fcount = 0;
static int dcount = 0;
static const char *datatop = NULL;

static void
teardown (void)
{
  for (int i = 0; i < lvaluesz; ++i) {
    if (lvalues [i].type == CHK_DIR) {
      diropDeleteDir (lvalues [i].name, DIROP_ALL);
    }
  }
  audiosrcCleanup ();
  bdjoptCleanup ();
  bdjvarsCleanup ();
}

static void
setup (void)
{
  FILE *fh;

  teardown ();

  datatop = sysvarsGetStr (SV_BDJ4_DIR_DATATOP);

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "/testpath");
  bdjvarsInit ();
  audiosrcInit ();

  for (int i = 0; i < lvaluesz; ++i) {
    lvalues [i].flag = 0;

    if (lvalues [i].type == CHK_FILE) {
      fh = fileopOpen (lvalues [i].name, "w");
      mdextfclose (fh);
      fclose (fh);
      ++fcount;
    }
    if (lvalues [i].type == CHK_DIR) {
      diropMakeDir (lvalues [i].name);
      ++dcount;
      ++dcount;
    }
  }

  --dcount; // don't count top level
}

START_TEST(audiosrc_gettype)
{
  int   rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- audiosrc_gettype");
  mdebugSubTag ("audiosrc_gettype");

  for (int i = 0; i < tvaluesz; ++i) {
    rc = audiosrcGetType (tvalues [i].test);
    if (rc != tvalues [i].type) {
      fprintf (stderr, "gt: %s %d\n", tvalues [i].test, rc);
    }
    ck_assert_int_eq (rc, tvalues [i].type);
  }
}
END_TEST

START_TEST(audiosrc_fullpath)
{
  char  tbuff [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- audiosrc_fullpath");
  mdebugSubTag ("audiosrc_fullpath");

  for (int i = 0; i < tvaluesz; ++i) {
    audiosrcFullPath (tvalues [i].test, tbuff, sizeof (tbuff), NULL, 0);
    ck_assert_str_eq (tbuff, tvalues [i].fullpath);
  }
}
END_TEST

START_TEST(audiosrc_relpath)
{
  const char *res;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- audiosrc_relpath");
  mdebugSubTag ("audiosrc_relpath");

  for (int i = 0; i < tvaluesz; ++i) {
    if (tvalues [i].type != AUDIOSRC_TYPE_FILE) {
      continue;
    }
    res = audiosrcRelativePath (tvalues [i].test, 0);
    ck_assert_str_eq (res, tvalues [i].relpath);
    res = audiosrcRelativePath (tvalues [i].fullpath, 0);
    ck_assert_str_eq (res, tvalues [i].relpath);
  }
}
END_TEST

START_TEST(audiosrc_exists)
{
  bool        val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- audiosrc_exists");
  mdebugSubTag ("audiosrc_exists");

  bdjoptSetStr (OPT_M_DIR_MUSIC, datatop);
  audiosrcPostInit ();

  val = audiosrcExists ("tmp/abc");
  ck_assert_int_eq (val, false);
  val = audiosrcExists (lvalues [1].name);
  ck_assert_int_eq (val, true);
  val = audiosrcExists (lvalues [12].name);
  ck_assert_int_eq (val, true);
}
END_TEST

START_TEST(audiosrc_hasorig)
{
  bool        val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- audiosrc_hasorig");
  mdebugSubTag ("audiosrc_hasorig");

  bdjoptSetStr (OPT_M_DIR_MUSIC, datatop);
  audiosrcPostInit ();

  val = audiosrcOriginalExists ("tmp/abc");
  ck_assert_int_eq (val, false);
  val = audiosrcOriginalExists (lvalues [1].name);
  ck_assert_int_eq (val, false);
  val = audiosrcOriginalExists ("tmp/abc/ghi/ghi.txt");
  ck_assert_int_eq (val, true);
}
END_TEST

START_TEST(audiosrc_prep)
{
  bool    val;
  char    tn [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- audiosrc_prep");
  mdebugSubTag ("audiosrc_prep");

  bdjoptSetStr (OPT_M_DIR_MUSIC, datatop);
  audiosrcPostInit ();

  for (int i = 0; i < lvaluesz; ++i) {
    if (lvalues [i].type == CHK_FILE) {
      audiosrcPrep (lvalues [i].name, tn, sizeof (tn));
      val = fileopFileExists (tn);
      ck_assert_int_eq (val, true);
      audiosrcPrepClean (lvalues [i].name, tn);
      val = fileopFileExists (tn);
      ck_assert_int_eq (val, false);
    }
  }
}
END_TEST

START_TEST(audiosrc_iterate)
{
  asiter_t    *asiter;
  int         c;
  const char  *val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- audiosrc_iterate");
  mdebugSubTag ("audiosrc_iterate");

  bdjoptSetStr (OPT_M_DIR_MUSIC, datatop);
  audiosrcPostInit ();

  asiter = audiosrcStartIterator (AUDIOSRC_TYPE_FILE, AS_ITER_DIR, "tmp/abc", NULL, -1);
  c = audiosrcIterCount (asiter);
  ck_assert_int_eq (c, fcount);

  c = 0;
  while ((val = audiosrcIterate (asiter)) != NULL) {
    for (int i = 0; i < lvaluesz; ++i) {
      if (strcmp (lvalues [i].name, val) == 0) {
        lvalues [i].flag = 1;
        break;
      }
    }
    ++c;
  }
  ck_assert_int_eq (c, fcount);
  for (int i = 0; i < lvaluesz; ++i) {
    if (lvalues [i].type == CHK_FILE) {
      ck_assert_int_eq (lvalues [i].flag, 1);
    }
  }

  audiosrcCleanIterator (asiter);
}
END_TEST

START_TEST(audiosrc_remove)
{
  bool        val;
  char        tmp [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- audiosrc_remove");
  mdebugSubTag ("audiosrc_remove");

  bdjoptSetStr (OPT_M_DIR_MUSIC, datatop);
  audiosrcPostInit ();

  val = audiosrcExists (lvalues [1].name);
  ck_assert_int_eq (val, true);
  val = audiosrcRemove (lvalues [1].name);
  ck_assert_int_eq (val, true);
  val = audiosrcExists (lvalues [1].name);
  ck_assert_int_eq (val, false);
  val = audiosrcRemove (lvalues [1].name);
  ck_assert_int_eq (val, false);
  snprintf (tmp, sizeof (tmp), "%s/%s%s",
      lvalues [0].name, bdjvarsGetStr (BDJV_DELETE_PFX), lvalues [1].fname);
  val = fileopFileExists (tmp);
  ck_assert_int_eq (val, true);

  val = audiosrcExists (lvalues [12].name);
  ck_assert_int_eq (val, true);
  val = audiosrcRemove (lvalues [12].name);
  ck_assert_int_eq (val, true);
  val = audiosrcExists (lvalues [12].name);
  ck_assert_int_eq (val, false);
  /* lvalues [10].name is the directory name */
  snprintf (tmp, sizeof (tmp), "%s/%s%s",
      lvalues [10].name, bdjvarsGetStr (BDJV_DELETE_PFX), lvalues [12].fname);
  val = fileopFileExists (tmp);
  ck_assert_int_eq (val, true);
}
END_TEST

START_TEST(audiosrc_iterate_src)
{
  asiter_t    *asiter;
  int         c;
  const char  *val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- audiosrc_iterate_src");
  mdebugSubTag ("audiosrc_iterate_src");

  bdjoptSetStr (OPT_M_DIR_MUSIC, datatop);
  audiosrcPostInit ();

  /* the test audiosrc file has three clients set up, two bdj4, one podcast */
  c = audiosrcGetActiveCount ();
  ck_assert_int_eq (c, 3);

  asiter = audiosrcStartIterator (AUDIOSRC_TYPE_NONE, AS_ITER_AUDIO_SRC, NULL, NULL, -1);
  c = audiosrcIterCount (asiter);
  ck_assert_int_eq (c, 3);

  c = 0;
  while ((val = audiosrcIterate (asiter)) != NULL) {
    if (strcmp (val, "BDJ4") == 0 || strcmp (val, "file") == 0) {
      ++c;
    }
    ++c;
  }
  ck_assert_int_eq (c, 5);

  audiosrcCleanIterator (asiter);
}
END_TEST

Suite *
audiosrc_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("audiosrc");
  tc = tcase_create ("audiosrc");
  tcase_set_tags (tc, "libaudiosrc");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, audiosrc_gettype);
  tcase_add_test (tc, audiosrc_fullpath);
  tcase_add_test (tc, audiosrc_relpath);
  tcase_add_test (tc, audiosrc_exists);
  tcase_add_test (tc, audiosrc_hasorig);
  tcase_add_test (tc, audiosrc_prep);
  tcase_add_test (tc, audiosrc_iterate);
  tcase_add_test (tc, audiosrc_remove);
  tcase_add_test (tc, audiosrc_iterate_src);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
