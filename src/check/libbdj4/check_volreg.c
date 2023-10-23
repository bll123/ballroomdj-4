/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

#include "bdjstring.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "datafile.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "pathbld.h"
#include "sysvars.h"
#include "volreg.h"

enum {
  VOLREG_SINK,
  VOLREG_COUNT,
  VOLREG_ORIG_VOL,
  VOLREG_KEY_MAX,
};

static datafilekey_t volregdfkeys [VOLREG_KEY_MAX] = {
  { "COUNT",              VOLREG_COUNT,     VALUE_NUM, NULL, DF_NORM },
  { "ORIGVOL",            VOLREG_ORIG_VOL,  VALUE_NUM, NULL, DF_NORM },
  { "SINK",               VOLREG_SINK,      VALUE_STR, NULL, DF_NORM },
};

#define TESTSINK  "quiet"
#define TESTSINKB  "silence"

static void checkSink (const char *fn, const char *testsink, int origvol, int count);

static void
setup (void)
{
  bdjvarsInit ();
}

static void
teardown (void)
{
  bdjvarsCleanup ();
}

START_TEST(volreg_save)
{
  char        tbuff [MAXPATHLEN];
  bool        exists;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- volreg_save");
  mdebugSubTag ("volreg_save");

  pathbldMakePath (tbuff, sizeof (tbuff),
      VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CACHE);
  unlink (tbuff);

  volregSave (TESTSINK, 76);

  exists = fileopFileExists (tbuff);
  ck_assert_int_eq (exists, 1);

  checkSink (tbuff, TESTSINK, 76, 1);
  volregSave (TESTSINK, 21);
  checkSink (tbuff, TESTSINK, 76, 2);

  volregSave (TESTSINKB, 21);
  checkSink (tbuff, TESTSINKB, 21, 1);
  checkSink (tbuff, TESTSINK, 76, 2);
}
END_TEST

START_TEST(volreg_clear)
{
  char    tbuff [MAXPATHLEN];
  int     val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- volreg_clear");
  mdebugSubTag ("volreg_clear");

  pathbldMakePath (tbuff, sizeof (tbuff),
      VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CACHE);

  checkSink (tbuff, TESTSINK, 76, 2);
  val = volregClear (TESTSINK);
  ck_assert_int_eq (val, -1);
  checkSink (tbuff, TESTSINK, 76, 1);
  val = volregClear (TESTSINK);
  ck_assert_int_eq (val, 76);
  checkSink (tbuff, TESTSINK, 76, 0);
}
END_TEST

START_TEST(volreg_bdj3flag)
{
  char    tbuff [MAXPATHLEN];
  bool    exists;
  FILE    *fh;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- volreg_bdj3flag");
  mdebugSubTag ("volreg_bdj3flag");

  pathbldMakePath (tbuff, sizeof (tbuff),
      VOLREG_BDJ3_EXT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CONFIG);
  fileopDelete (tbuff);

  exists = volregCheckBDJ3Flag ();
  ck_assert_int_eq (exists, 0);
  fh = fopen (tbuff, "w");
  if (fh != NULL) {
    fclose (fh);
  }
  exists = volregCheckBDJ3Flag ();
  ck_assert_int_eq (exists, 1);
  fileopDelete (tbuff);
}
END_TEST

START_TEST(volreg_bdj4flag)
{
  char    tbuff [MAXPATHLEN];
  bool    exists;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- volreg_bdj4flag");
  mdebugSubTag ("volreg_bdj4flag");

  pathbldMakePath (tbuff, sizeof (tbuff),
      VOLREG_BDJ4_EXT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CONFIG);

  exists = fileopFileExists (tbuff);
  if (exists) {
    volregClearBDJ4Flag ();
  }

  exists = fileopFileExists (tbuff);
  ck_assert_int_eq (exists, 0);

  volregCreateBDJ4Flag ();
  exists = fileopFileExists (tbuff);
  ck_assert_int_eq (exists, 1);
  volregClearBDJ4Flag ();
  exists = fileopFileExists (tbuff);
  ck_assert_int_eq (exists, 0);
  fileopDelete (tbuff);
}
END_TEST

Suite *
volreg_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("volreg");
  tc = tcase_create ("volreg");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, volreg_save);
  tcase_add_test (tc, volreg_clear);
  tcase_add_test (tc, volreg_bdj3flag);
  tcase_add_test (tc, volreg_bdj4flag);
  suite_add_tcase (s, tc);

  return s;
}

static void
checkSink (const char *fn, const char *testsink, int origvol, int count)
{
  datafile_t  *df;
  ilist_t     *vlist;
  ilistidx_t  iteridx;
  ilistidx_t  idx;
  const char  *sink;
  int         val;

  df = datafileAllocParse ("volreg", DFTYPE_INDIRECT, fn,
      volregdfkeys, VOLREG_KEY_MAX, DF_NO_OFFSET, NULL);
  vlist = datafileGetList (df);
  ilistStartIterator (vlist, &iteridx);
  while ((idx = ilistIterateKey (vlist, &iteridx)) >= 0) {
    sink = ilistGetStr (vlist, idx, VOLREG_SINK);
    if (strcmp (sink, testsink) == 0) {
      val = ilistGetNum (vlist, idx, VOLREG_ORIG_VOL);
      ck_assert_int_eq (val, origvol);
      val = ilistGetNum (vlist, idx, VOLREG_COUNT);
      ck_assert_int_eq (val, count);
      break;
    }
  }
  datafileFree (df);
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
