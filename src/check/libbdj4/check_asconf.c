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

#include "asconf.h"
#include "audiosrc.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "datafile.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "templateutil.h"

static void
setup (void)
{
  templateFileCopy ("audiosrc.txt", "audiosrc.txt");
}

static void
teardown (void)
{
  return;
}

START_TEST(asconf_alloc)
{
  asconf_t   *asconf = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- asconf_alloc");
  mdebugSubTag ("asconf_alloc");

  asconf = asconfAlloc ();
  ck_assert_ptr_nonnull (asconf);
  asconfFree (asconf);
}
END_TEST

START_TEST(asconf_iterate)
{
  asconf_t     *asconf = NULL;
  slistidx_t  iteridx;
  int         count;
  int         key;
  int         val;
  const char  *sval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- asconf_iterate");
  mdebugSubTag ("asconf_iterate");

  asconf = asconfAlloc ();
  asconfStartIterator (asconf, &iteridx);
  count = 0;
  while ((key = asconfIterate (asconf, &iteridx)) >= 0) {
    val = asconfGetNum (asconf, key, ASCONF_MODE);
    ck_assert_int_eq (val, ASCONF_MODE_OFF);
    val = asconfGetNum (asconf, key, ASCONF_TYPE);
    ck_assert_int_eq (val, AUDIOSRC_TYPE_BDJ4);
    sval = asconfGetStr (asconf, key, ASCONF_NAME);
    ck_assert_str_eq (sval, "BDJ4");
    val = asconfGetNum (asconf, key, ASCONF_PORT);
    ck_assert_int_eq (val, 9011);
    ++count;
  }
  ck_assert_int_eq (count, asconfGetCount (asconf));
  asconfFree (asconf);
}
END_TEST


START_TEST(asconf_set)
{
  asconf_t    *asconf = NULL;
  char        *sval = NULL;
  const char  *tsval;
  slistidx_t  iteridx;
  int         count;
  int         key;
  int         val;
  int         tval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- asconf_set");
  mdebugSubTag ("asconf_set");

  bdjvarsdfloadInit ();

  asconf = asconfAlloc ();
  asconfStartIterator (asconf, &iteridx);
  count = 0;
  while ((key = asconfIterate (asconf, &iteridx)) >= 0) {
    sval = mdstrdup (asconfGetStr (asconf, key, ASCONF_NAME));
    asconfSetStr (asconf, key, ASCONF_NAME, "abc123");
    tsval = asconfGetStr (asconf, key, ASCONF_NAME);
    ck_assert_ptr_nonnull (tsval);
    ck_assert_str_ne (sval, tsval);
    ck_assert_str_eq (tsval, "abc123");
    asconfSetStr (asconf, key, ASCONF_NAME, sval);
    mdfree (sval);

    val = asconfGetNum (asconf, key, ASCONF_MODE);
    asconfSetNum (asconf, key, ASCONF_MODE, ASCONF_MODE_CLIENT);
    tval = asconfGetNum (asconf, key, ASCONF_MODE);
    ck_assert_int_eq (tval, ASCONF_MODE_CLIENT);
    asconfSetNum (asconf, key, ASCONF_MODE, val);

    val = asconfGetNum (asconf, key, ASCONF_TYPE);
    asconfSetNum (asconf, key, ASCONF_TYPE, AUDIOSRC_TYPE_RTSP);
    tval = asconfGetNum (asconf, key, ASCONF_TYPE);
    ck_assert_int_eq (tval, AUDIOSRC_TYPE_RTSP);
    asconfSetNum (asconf, key, ASCONF_TYPE, val);

    asconfSetStr (asconf, key, ASCONF_USER, "bdj4");
    tsval = asconfGetStr (asconf, key, ASCONF_USER);
    ck_assert_str_eq (tsval, "bdj4");

    asconfSetStr (asconf, key, ASCONF_PASS, "bdj");
    tsval = asconfGetStr (asconf, key, ASCONF_PASS);
    ck_assert_str_eq (tsval, "bdj");

    asconfSetStr (asconf, key, ASCONF_URI, "localhost");
    tsval = asconfGetStr (asconf, key, ASCONF_URI);
    ck_assert_str_eq (tsval, "localhost");

    ++count;
  }
  ck_assert_int_eq (count, asconfGetCount (asconf));

  asconfFree (asconf);
  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(asconf_save)
{
  asconf_t     *asconf = NULL;
  const char  *sval = NULL;
  const char  *tsval = NULL;
  int         val;
  int         tval;
  ilistidx_t  iteridx;
  ilistidx_t  titeridx;
  ilist_t     *tlist;
  int         key;
  int         tkey;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- asconf_save");
  mdebugSubTag ("asconf_save");

  asconf = asconfAlloc ();

  asconfStartIterator (asconf, &iteridx);
  tlist = ilistAlloc ("chk-asconf-a", LIST_ORDERED);
  while ((key = asconfIterate (asconf, &iteridx)) >= 0) {
    sval = asconfGetStr (asconf, key, ASCONF_NAME);
    val = asconfGetNum (asconf, key, ASCONF_TYPE);

    asconfSetStr (asconf, key, ASCONF_USER, "bdj4");
    asconfSetStr (asconf, key, ASCONF_PASS, "bdj");
    asconfSetStr (asconf, key, ASCONF_URI, "localhost");

    ilistSetStr (tlist, key, ASCONF_NAME, sval);
    ilistSetNum (tlist, key, ASCONF_TYPE, val);
    ilistSetStr (tlist, key, ASCONF_USER, "bdj4");
    ilistSetStr (tlist, key, ASCONF_PASS, "bdj");
    ilistSetStr (tlist, key, ASCONF_URI, "localhost");
  }

  asconfSave (asconf, tlist, -1);
  asconfFree (asconf);

  asconf = asconfAlloc ();

  ck_assert_int_eq (ilistGetCount (tlist), asconfGetCount (asconf));

  asconfStartIterator (asconf, &iteridx);
  ilistStartIterator (tlist, &titeridx);
  while ((key = asconfIterate (asconf, &iteridx)) >= 0) {
    tkey = ilistIterateKey (tlist, &titeridx);
    ck_assert_int_eq (key, tkey);

    sval = asconfGetStr (asconf, key, ASCONF_NAME);
    tsval = ilistGetStr (tlist, key, ASCONF_NAME);
    ck_assert_str_eq (sval, tsval);

    val = asconfGetNum (asconf, key, ASCONF_TYPE);
    tval = ilistGetNum (tlist, key, ASCONF_TYPE);
    ck_assert_int_eq (val, tval);

    sval = asconfGetStr (asconf, key, ASCONF_USER);
    tsval = ilistGetStr (tlist, key, ASCONF_USER);
    ck_assert_str_eq (sval, tsval);

    sval = asconfGetStr (asconf, key, ASCONF_PASS);
    tsval = ilistGetStr (tlist, key, ASCONF_PASS);
    ck_assert_str_eq (sval, tsval);

    sval = asconfGetStr (asconf, key, ASCONF_URI);
    tsval = ilistGetStr (tlist, key, ASCONF_URI);
    ck_assert_str_eq (sval, tsval);
  }

  ilistFree (tlist);
  asconfFree (asconf);
}
END_TEST

START_TEST(asconf_add)
{
  asconf_t   *asconf = NULL;
  ilistidx_t  key;
  ilistidx_t  iteridx;
  int         rc;
  int         val;
  const char  *sval = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- asconf_add");
  mdebugSubTag ("asconf_add");

  asconf = asconfAlloc ();

  asconfAdd (asconf, "test");

  asconfStartIterator (asconf, &iteridx);
  rc = 0;
  while ((key = asconfIterate (asconf, &iteridx)) >= 0) {
    sval = asconfGetStr (asconf, key, ASCONF_NAME);
    if (strcmp (sval, "test") == 0) {
      rc += 1;
      val = asconfGetNum(asconf, key, ASCONF_TYPE);
      if (val == AUDIOSRC_TYPE_RTSP) {
        rc += 1;
      }
    }
  }
  ck_assert_int_eq (rc, 2);

  asconfFree (asconf);
}
END_TEST


START_TEST(asconf_delete)
{
  asconf_t     *asconf = NULL;
  ilistidx_t  key;
  ilistidx_t  iteridx;
  int         count;
  int         val;
  int         nval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- asconf_delete");
  mdebugSubTag ("asconf_delete");

  asconf = asconfAlloc ();

  val = asconfGetCount (asconf);

  count = 0;
  asconfStartIterator (asconf, &iteridx);
  while ((key = asconfIterate (asconf, &iteridx)) >= 0) {
    asconfDelete (asconf, key);
    ++count;
    break;
  }
  ck_assert_int_eq (count, val);
  nval = asconfGetCount (asconf);
  ck_assert_int_eq (val - 1, nval);

  asconfFree (asconf);
}
END_TEST

Suite *
asconf_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("asconf");
  tc = tcase_create ("asconf");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, asconf_alloc);
  tcase_add_test (tc, asconf_iterate);
  tcase_add_test (tc, asconf_set);
  tcase_add_test (tc, asconf_save);
  tcase_add_test (tc, asconf_add);
  tcase_add_test (tc, asconf_delete);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
