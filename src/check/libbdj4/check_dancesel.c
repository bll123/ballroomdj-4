/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "audiosrc.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "bdjvarsdf.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "dance.h"
#include "dancesel.h"
#include "filemanip.h"
#include "log.h"
#include "musicdb.h"
#include "nlist.h"
#include "slist.h"
#include "templateutil.h"

#define DANCESEL_DEBUG 0

enum {
  TM_MAX_DANCE = 20,        // normally 14 or so in the standard template.
};

static char *dbfn = "data/musicdb.dat";
static musicdb_t  *db = NULL;
static nlist_t    *ghist = NULL;
static int        gprior = 0;

static void saveToQueue (ilistidx_t idx);
static ilistidx_t chkQueue (void *udata, ilistidx_t idx);

static void
setup (void)
{
  templateFileCopy ("autoselection.txt", "autoselection.txt");
  templateFileCopy ("dancetypes.txt", "dancetypes.txt");
  templateFileCopy ("dances.txt", "dances.txt");
  templateFileCopy ("genres.txt", "genres.txt");
  templateFileCopy ("levels.txt", "levels.txt");
  templateFileCopy ("ratings.txt", "ratings.txt");
  templateFileCopy ("sortopt.txt", "sortopt.txt");
  filemanipCopy ("test-templates/status.txt", "data/status.txt");
  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_NONE);
  bdjvarsInit ();
  bdjvarsdfloadInit ();
  audiosrcInit ();
  db = dbOpen (dbfn);
}

static void
teardown (void)
{
  dbClose (db);
  audiosrcCleanup ();
  bdjvarsdfloadCleanup ();
  bdjvarsCleanup ();
  bdjoptCleanup ();
}

START_TEST(dancesel_alloc)
{
  dancesel_t  *ds;
  nlist_t     *clist = NULL;
  dance_t     *dances;
  slist_t     *dlist;
  ilistidx_t  wkey;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_alloc");
  mdebugSubTag ("dancesel_alloc");
#if DANCESEL_DEBUG
    fprintf (stderr, "--chk-- dancesel_alloc\n");
#endif

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  nlistSetNum (clist, wkey, 5);
  /* it doesn't crash with no data, but avoid silliness */
  ds = danceselAlloc (clist, NULL, NULL);
  danceselFree (ds);
  nlistFree (clist);
}
END_TEST

START_TEST(dancesel_choose_single_no_hist)
{
  dancesel_t  *ds;
  nlist_t     *clist = NULL;
  dance_t     *dances;
  slist_t     *dlist;
  ilistidx_t  wkey;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_single_no_hist");
  mdebugSubTag ("dancesel_choose_single_no_hist");
#if DANCESEL_DEBUG
    fprintf (stderr, "-- dancesel_choose_single_no_hist\n");
#endif

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  nlistSetNum (clist, wkey, 5);
  ds = danceselAlloc (clist, NULL, NULL);

  for (int i = 0; i < 16; ++i) {
    ilistidx_t  didx;

    didx = danceselSelect (ds, 0);
    danceselAddCount (ds, didx);
    ck_assert_int_eq (didx, wkey);
  }

  danceselFree (ds);
  nlistFree (clist);
}
END_TEST

START_TEST(dancesel_choose_three_no_hist)
{
  dancesel_t  *ds;
  nlist_t     *clist = NULL;
  dance_t     *dances;
  slist_t     *dlist;
  ilistidx_t  wkey, tkey, fkey;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_three_no_hist");
  mdebugSubTag ("dancesel_choose_three_no_hist");
#if DANCESEL_DEBUG
    fprintf (stderr, "-- dancesel_choose_three_no_hist\n");
#endif

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");
  tkey = slistGetNum (dlist, "Tango");
  fkey = slistGetNum (dlist, "Foxtrot");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  nlistSetNum (clist, wkey, 2);
  nlistSetNum (clist, tkey, 2);
  nlistSetNum (clist, fkey, 2);
  ds = danceselAlloc (clist, NULL, NULL);

  for (int i = 0; i < 16; ++i) {
    ilistidx_t  didx;
    int         rc;

    didx = danceselSelect (ds, 0);
    danceselAddCount (ds, didx);

    rc = didx == wkey || didx == tkey || didx == fkey;;
    ck_assert_int_eq (rc, 1);
  }

  danceselFree (ds);
  nlistFree (clist);
}
END_TEST

/* the following are checking against the queue */
/* dance selection doesn't work well due to the limited number of dances */
/* being selected in these tests */

START_TEST(dancesel_choose_two_hist_s)
{
  dancesel_t  *ds;
  nlist_t     *clist = NULL;
  dance_t     *dances;
  slist_t     *dlist;
  ilistidx_t  wkey, tkey;
  ilistidx_t  lastdidx;
  int         count;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_two_hist_s");
  mdebugSubTag ("dancesel_choose_two_hist_s");
#if DANCESEL_DEBUG
    fprintf (stderr, "-- dancesel_choose_two_hist_s\n");
#endif

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");
  tkey = slistGetNum (dlist, "Tango");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  /* a symmetric outcome */
  nlistSetNum (clist, wkey, 4);
  nlistSetNum (clist, tkey, 4);
  ds = danceselAlloc (clist, chkQueue, NULL);

  lastdidx = -1;
  gprior = 0;
  count = 0;
  for (int i = 0; i < 50; ++i) {
    ilistidx_t  didx;
    int         rc;

    didx = danceselSelect (ds, gprior);
    rc = didx == wkey || didx == tkey;
    ck_assert_int_eq (rc, 1);
    /* with only two dances and the same counts, they should alternate */
    if (didx == lastdidx) {
      ++count;
    }
    danceselAddCount (ds, didx);
    danceselAddPlayed (ds, didx);
    saveToQueue (didx);
    lastdidx = didx;
  }
  ck_assert_int_lt (count, 11);

  danceselFree (ds);
  nlistFree (clist);
  nlistFree (ghist);
  ghist = NULL;
}
END_TEST

START_TEST(dancesel_choose_two_hist_a)
{
  dancesel_t  *ds;
  nlist_t     *clist = NULL;
  dance_t     *dances;
  slist_t     *dlist;
  ilistidx_t  wkey, tkey;
  int         counts [TM_MAX_DANCE];
  int         val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_two_hist_a");
  mdebugSubTag ("dancesel_choose_two_hist_a");
#if DANCESEL_DEBUG
    fprintf (stderr, "-- dancesel_choose_two_hist_a\n");
#endif

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");
  tkey = slistGetNum (dlist, "Tango");

  for (int i = 0; i < TM_MAX_DANCE; ++i) {
    counts [i] = 0;
  }

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  /* create an asymmetric count list */
  nlistSetNum (clist, wkey, 2);
  nlistSetNum (clist, tkey, 4);
  ds = danceselAlloc (clist, chkQueue, NULL);

  gprior = 0;
  for (int i = 0; i < 50; ++i) {
    ilistidx_t  didx;
    int         rc;

    didx = danceselSelect (ds, gprior);
    rc = didx == wkey || didx == tkey;
    ck_assert_int_eq (rc, 1);
    counts [didx]++;
    danceselAddCount (ds, didx);
    danceselAddPlayed (ds, didx);
    saveToQueue (didx);
  }
  /* with only two dances, the chances of a non-symmetric outcome */
  /* are low, but happen, so test a range */
  val = abs (counts [wkey] - counts [tkey]);
  ck_assert_int_le (val, 6);

  danceselFree (ds);
  nlistFree (clist);
  nlistFree (ghist);
  ghist = NULL;
}
END_TEST

/* a basic check for counts and tags */

START_TEST(dancesel_choose_multi_count)
{
  dancesel_t  *ds;
  nlist_t     *clist = NULL;
  dance_t     *dances;
  slist_t     *dlist;
  ilistidx_t  wkey, tkey, rkey;
  int         counts [TM_MAX_DANCE];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_multi_count");
  mdebugSubTag ("dancesel_choose_multi_count");
#if DANCESEL_DEBUG
    fprintf (stderr, "-- dancesel_choose_multi_count\n");
#endif

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");
  tkey = slistGetNum (dlist, "Tango");
  rkey = slistGetNum (dlist, "Rumba");

  for (int i = 0; i < TM_MAX_DANCE; ++i) {
    counts [i] = 0;
  }

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  /* create an asymmetric count list */
  nlistSetNum (clist, wkey, 4);
  nlistSetNum (clist, tkey, 4);
  nlistSetNum (clist, rkey, 8);
  ds = danceselAlloc (clist, chkQueue, NULL);

  gprior = 0;
  for (int i = 0; i < 16; ++i) {
    ilistidx_t  didx;
    int         rc;

    didx = danceselSelect (ds, gprior);
    rc = didx == wkey || didx == tkey || didx == rkey;
    ck_assert_int_eq (rc, 1);
    counts [didx]++;
    danceselAddCount (ds, didx);
    danceselAddPlayed (ds, didx);
    saveToQueue (didx);
  }
  /* check for a range */
  /* these tests are non-deterministic, there's still a possibility of a */
  /* failure */
  ck_assert_int_le (counts [wkey], 5);
  ck_assert_int_le (counts [tkey], 6);
  ck_assert_int_le (counts [rkey], 8);

  danceselFree (ds);
  nlistFree (clist);
  nlistFree (ghist);
  ghist = NULL;
}
END_TEST

/* another tag check */

START_TEST(dancesel_choose_multi_tag)
{
  dancesel_t  *ds;
  nlist_t     *clist = NULL;
  dance_t     *dances;
  slist_t     *dlist;
  ilistidx_t  wkey, jkey, wcskey, rkey, fkey;
  ilistidx_t  lastdidx;
  int         count;

  /* it is difficult to ascertain whether the tag match count is low enough */
  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_multi_tag");
  mdebugSubTag ("dancesel_choose_multi_tag");
#if DANCESEL_DEBUG
    fprintf (stderr, "-- dancesel_choose_multi_tag\n");
#endif

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  fkey = slistGetNum (dlist, "Foxtrot");
  wkey = slistGetNum (dlist, "Waltz");
  jkey = slistGetNum (dlist, "Jive");
  wcskey = slistGetNum (dlist, "West Coast Swing");
  rkey = slistGetNum (dlist, "Rumba");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  nlistSetNum (clist, wkey, 20);
  nlistSetNum (clist, jkey, 20);
  nlistSetNum (clist, wcskey, 20);
  nlistSetNum (clist, rkey, 20);
  nlistSetNum (clist, fkey, 20);
  ds = danceselAlloc (clist, chkQueue, NULL);

  count = 0;
  lastdidx = -1;
  gprior = 0;
  for (int i = 0; i < 50; ++i) {
    ilistidx_t  didx;
    int         rc;

    didx = danceselSelect (ds, gprior);
    rc = didx == wkey || didx == jkey || didx == wcskey ||
        didx == rkey || didx == fkey;
    ck_assert_int_eq (rc, 1);
    ck_assert_int_ne (didx, lastdidx);
    /* the tag match should help prevent a jive and a wcs */
    /* next to each other, but not always */
    if (didx == wcskey || didx == jkey) {
      if (lastdidx == jkey || lastdidx == wcskey) {
        ++count;
      }
    }
    lastdidx = didx;
    danceselAddCount (ds, didx);
    danceselAddPlayed (ds, didx);
    saveToQueue (didx);
  }
  /* there are too many variables to determine an exact value */
  /* but there should not be too many jive/wcs next to each other */
  ck_assert_int_lt (count, 10);

  danceselFree (ds);
  nlistFree (clist);
  nlistFree (ghist);
  ghist = NULL;
}
END_TEST

/* fast dance check */
/* test both the begin-fast limit and fast/fast */

START_TEST(dancesel_choose_fast)
{
  dancesel_t  *ds;
  nlist_t     *clist = NULL;
  dance_t     *dances;
  slist_t     *dlist;
  ilistidx_t  wkey, tkey, rkey, qskey, jkey, fkey;
  ilistidx_t  lastfast;
  int         fastmatch;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_fast");
  mdebugSubTag ("dancesel_choose_fast");
#if DANCESEL_DEBUG
    fprintf (stderr, "-- dancesel_choose_fast\n");
#endif

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  /* fast dances: jive, salsa, quickstep, viennese waltz */
  /* mixing viennese waltz into this test makes a mess, as it matches */
  /* too many of the other dances */
  jkey = slistGetNum (dlist, "Jive");
  qskey = slistGetNum (dlist, "Quickstep");
  wkey = slistGetNum (dlist, "Waltz");
  tkey = slistGetNum (dlist, "Tango");
  fkey = slistGetNum (dlist, "Foxtrot");
  rkey = slistGetNum (dlist, "Rumba");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  nlistSetNum (clist, jkey, 6);
  nlistSetNum (clist, qskey, 6);
  nlistSetNum (clist, fkey, 6);
  nlistSetNum (clist, wkey, 6);
  nlistSetNum (clist, tkey, 6);
  nlistSetNum (clist, rkey, 6);
  ds = danceselAlloc (clist, chkQueue, NULL);

  lastfast = 0;
  gprior = 0;
  fastmatch = 0;
  for (int i = 0; i < 50; ++i) {
    ilistidx_t  didx;
    int         fast;
    int         notfast;

    didx = danceselSelect (ds, gprior);
    fast = didx == jkey || didx == qskey;
    notfast = didx == wkey || didx == rkey || didx == fkey || didx == tkey;
    ck_assert_int_eq (fast | notfast, 1);
    if (i < 3) {
      if (fast != 0) {
        ++fastmatch;
      }
      ck_assert_int_eq (notfast, 1);
    }
    if (i >= 3 && (lastfast || fast)) {
      /* fast dances never appear next to each other */
      if (lastfast == fast) {
        ++fastmatch;
      }
    }
    lastfast = fast;
    danceselAddCount (ds, didx);
    danceselAddPlayed (ds, didx);
    saveToQueue (didx);
  }
  /* fast songs should be extremely infrequent */
  ck_assert_int_lt (fastmatch, 2);

  danceselFree (ds);
  nlistFree (clist);
  nlistFree (ghist);
  ghist = NULL;
}
END_TEST

/* use the same logic as a mix, using the dance selection decrement */
/* routine. */
START_TEST(dancesel_mix)
{
  dancesel_t  *ds;
  nlist_t     *clist = NULL;
  dance_t     *dances;
  slist_t     *dlist;
  int         didxarr [8];
  int         counts [TM_MAX_DANCE];
  int         totcount;
  int         i;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_fast");
  mdebugSubTag ("dancesel_choose_fast");
#if DANCESEL_DEBUG
    fprintf (stderr, "-- dancesel_choose_fast\n");
#endif

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);

  i = 0;
  didxarr [i++] = slistGetNum (dlist, "Jive");
  didxarr [i++] = slistGetNum (dlist, "Quickstep");
  didxarr [i++] = slistGetNum (dlist, "Waltz");
  didxarr [i++] = slistGetNum (dlist, "Tango");
  didxarr [i++] = slistGetNum (dlist, "Foxtrot");
  didxarr [i++] = slistGetNum (dlist, "Rumba");
  didxarr [i++] = slistGetNum (dlist, "Salsa");
  didxarr [i++] = slistGetNum (dlist, "Cha Cha");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  for (int i = 0; i < 8; ++i) {
    nlistSetNum (clist, didxarr [i], 6);
  }
  for (int i = 0; i < TM_MAX_DANCE; ++i) {
    counts [i] = 0;
  }

  ds = danceselAlloc (clist, chkQueue, NULL);

  gprior = 0;
  totcount = 0;
  for (int i = 0; i < 48; ++i) {
    ilistidx_t  didx;

    didx = danceselSelect (ds, gprior);
    if (didx >= 0) {
      ++counts [didx];
      danceselAddCount (ds, didx);
      danceselDecrementBase (ds, didx);
      saveToQueue (didx);
      ++totcount;
    }
  }
  ck_assert_int_eq (totcount, 48);
  for (int i = 0; i < 8; ++i) {
    ilistidx_t didx = didxarr [i];
    ck_assert_int_eq (counts [didx], 6);
  }

  danceselFree (ds);
  nlistFree (clist);
  nlistFree (ghist);
  ghist = NULL;
}
END_TEST

Suite *
dancesel_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("dancesel");
  tc = tcase_create ("dancesel");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, dancesel_alloc);
  tcase_add_test (tc, dancesel_choose_single_no_hist);
  tcase_add_test (tc, dancesel_choose_three_no_hist);
  tcase_add_test (tc, dancesel_choose_two_hist_s);
  tcase_add_test (tc, dancesel_choose_two_hist_a);
  tcase_add_test (tc, dancesel_choose_multi_count);
  tcase_add_test (tc, dancesel_choose_multi_tag);
  tcase_add_test (tc, dancesel_choose_fast);
  tcase_add_test (tc, dancesel_mix);
  suite_add_tcase (s, tc);
  return s;
}

static void
saveToQueue (ilistidx_t idx)
{
  if (ghist == NULL) {
    ghist = nlistAlloc ("hist", LIST_UNORDERED, NULL);
  }
  nlistSetNum (ghist, idx, 0);
  gprior++;
}

static ilistidx_t
chkQueue (void *udata, ilistidx_t idx)
{
  ilistidx_t    didx;

  didx = nlistGetKeyByIdx (ghist, idx);
  return didx;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
