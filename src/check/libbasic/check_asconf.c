/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
  return;
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
  int         type;
  const char  *sval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- asconf_iterate");
  mdebugSubTag ("asconf_iterate");

  asconf = asconfAlloc ();
  asconfStartIterator (asconf, &iteridx);
  count = 0;
  while ((key = asconfIterate (asconf, &iteridx)) >= 0) {
    /* check get-list functions */
    val = asconfGetListASKey (asconf, count);
    ck_assert_int_eq (val, key);

    type = asconfGetNum (asconf, key, ASCONF_TYPE);
    val = asconfGetNum (asconf, key, ASCONF_MODE);
    ck_assert_int_eq (val, ASCONF_MODE_CLIENT);
    if (count == 0) {
      sval = asconfGetStr (asconf, key, ASCONF_NAME);
      ck_assert_str_eq (sval, "AAA");
    }
    val = asconfGetNum (asconf, key, ASCONF_PORT);
    if (type == AUDIOSRC_TYPE_BDJ4) {
      ck_assert_int_eq (val, 9011);
    }
    if (type == AUDIOSRC_TYPE_PODCAST) {
      ck_assert_int_eq (val, 443);
    }
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
    asconfSetNum (asconf, key, ASCONF_MODE, ASCONF_MODE_SERVER);
    tval = asconfGetNum (asconf, key, ASCONF_MODE);
    ck_assert_int_eq (tval, ASCONF_MODE_SERVER);
    asconfSetNum (asconf, key, ASCONF_MODE, val);

    val = asconfGetNum (asconf, key, ASCONF_TYPE);
    asconfSetNum (asconf, key, ASCONF_TYPE, AUDIOSRC_TYPE_NONE);
    tval = asconfGetNum (asconf, key, ASCONF_TYPE);
    ck_assert_int_eq (tval, AUDIOSRC_TYPE_NONE);
    asconfSetNum (asconf, key, ASCONF_TYPE, val);

    sval = mdstrdup (asconfGetStr (asconf, key, ASCONF_USER));
    asconfSetStr (asconf, key, ASCONF_USER, "bdj4");
    tsval = asconfGetStr (asconf, key, ASCONF_USER);
    ck_assert_str_eq (tsval, "bdj4");
    asconfSetStr (asconf, key, ASCONF_USER, sval);
    mdfree (sval);

    sval = mdstrdup (asconfGetStr (asconf, key, ASCONF_PASS));
    asconfSetStr (asconf, key, ASCONF_PASS, "bdj4");
    tsval = asconfGetStr (asconf, key, ASCONF_PASS);
    ck_assert_str_eq (tsval, "bdj4");
    asconfSetStr (asconf, key, ASCONF_PASS, sval);
    mdfree (sval);

    sval = mdstrdup (asconfGetStr (asconf, key, ASCONF_URI));
    asconfSetStr (asconf, key, ASCONF_URI, "localhost");
    tsval = asconfGetStr (asconf, key, ASCONF_URI);
    ck_assert_str_eq (tsval, "localhost");
    asconfSetStr (asconf, key, ASCONF_URI, sval);
    mdfree (sval);

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
    ilistSetNum (tlist, key, ASCONF_MODE,
        asconfGetNum (asconf, key, ASCONF_MODE));
    ilistSetNum (tlist, key, ASCONF_TYPE,
        asconfGetNum (asconf, key, ASCONF_TYPE));
    ilistSetStr (tlist, key, ASCONF_NAME,
        asconfGetStr (asconf, key, ASCONF_NAME));
    ilistSetStr (tlist, key, ASCONF_URI,
        asconfGetStr (asconf, key, ASCONF_URI));
    ilistSetNum (tlist, key, ASCONF_PORT,
        asconfGetNum (asconf, key, ASCONF_PORT));
    ilistSetStr (tlist, key, ASCONF_USER,
        asconfGetStr (asconf, key, ASCONF_USER));
    ilistSetStr (tlist, key, ASCONF_PASS,
        asconfGetStr (asconf, key, ASCONF_PASS));
  }

  asconfSave (asconf, tlist, -1);
  asconfFree (asconf);

  asconf = asconfAlloc ();

  ck_assert_int_eq (ilistGetCount (tlist), asconfGetCount (asconf));

  asconfStartIterator (asconf, &iteridx);
  ilistStartIterator (tlist, &titeridx);
  while ((key = asconfIterate (asconf, &iteridx)) >= 0) {
    tkey = ilistIterateKey (tlist, &titeridx);

    val = asconfGetNum (asconf, key, ASCONF_MODE);
    tval = ilistGetNum (tlist, key, ASCONF_MODE);
    ck_assert_int_eq (val, tval);

    val = asconfGetNum (asconf, key, ASCONF_TYPE);
    tval = ilistGetNum (tlist, key, ASCONF_TYPE);
    ck_assert_int_eq (val, tval);

    sval = asconfGetStr (asconf, key, ASCONF_NAME);
    tsval = ilistGetStr (tlist, key, ASCONF_NAME);
    ck_assert_str_eq (sval, tsval);

    sval = asconfGetStr (asconf, key, ASCONF_URI);
    tsval = ilistGetStr (tlist, key, ASCONF_URI);
    ck_assert_str_eq (sval, tsval);

    val = asconfGetNum (asconf, key, ASCONF_PORT);
    tval = ilistGetNum (tlist, key, ASCONF_PORT);
    ck_assert_int_eq (val, tval);

    sval = asconfGetStr (asconf, key, ASCONF_USER);
    tsval = ilistGetStr (tlist, key, ASCONF_USER);
    ck_assert_str_eq (sval, tsval);

    sval = asconfGetStr (asconf, key, ASCONF_PASS);
    tsval = ilistGetStr (tlist, key, ASCONF_PASS);
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

  rc = asconfGetCount (asconf);
  val = asconfAdd (asconf, "test");
  ck_assert_int_eq (rc, val);
  rc = asconfGetCount (asconf);
  ck_assert_int_eq (rc, 4);

  rc = asconfGetCount (asconf);
  val = asconfAdd (asconf, "testb");
  ck_assert_int_eq (rc, val);
  rc = asconfGetCount (asconf);
  ck_assert_int_eq (rc, 5);

  asconfStartIterator (asconf, &iteridx);
  rc = 0;
  while ((key = asconfIterate (asconf, &iteridx)) >= 0) {
    sval = asconfGetStr (asconf, key, ASCONF_NAME);
    if (strncmp (sval, "test", 4) == 0) {
      rc += 1;
      val = asconfGetNum(asconf, key, ASCONF_TYPE);
      if (val == AUDIOSRC_TYPE_BDJ4) {
        rc += 1;
      }
    }
  }
  ck_assert_int_eq (rc, 4);

  asconfFree (asconf);
}
END_TEST

START_TEST(asconf_delete)
{
  asconf_t     *asconf = NULL;
  ilistidx_t  key;
  ilistidx_t  iteridx;
  int         val;
  int         nval;
  const char  *sval;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- asconf_delete");
  mdebugSubTag ("asconf_delete");

  asconf = asconfAlloc ();
  asconfAdd (asconf, "test");

  val = asconfGetCount (asconf);
  ck_assert_int_eq (val, 4);

  asconfStartIterator (asconf, &iteridx);
  while ((key = asconfIterate (asconf, &iteridx)) >= 0) {
    sval = asconfGetStr (asconf, key, ASCONF_NAME);
    if (strcmp (sval, "test") == 0) {
      asconfDelete (asconf, key);
      break;
    }
  }

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
  tcase_set_tags (tc, "libbasic");
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
