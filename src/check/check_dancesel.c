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

#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "dance.h"
#include "dancesel.h"
#include "filemanip.h"
#include "log.h"
#include "musicdb.h"
#include "nlist.h"
#include "slist.h"
#include "templateutil.h"

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
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);
}

static void
teardown (void)
{
  dbClose (db);
  bdjvarsdfloadCleanup ();
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

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  nlistSetNum (clist, wkey, 5);
  /* it doesn't crash with no data, but avoid silliness */
  ds = danceselAlloc (clist);
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
// fprintf (stderr, "-- dancesel_choose_single_no_hist\n");

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  nlistSetNum (clist, wkey, 5);
  ds = danceselAlloc (clist);

  for (int i = 0; i < 16; ++i) {
    ilistidx_t  didx;

    didx = danceselSelect (ds, clist, 0, NULL, NULL);
// fprintf (stderr, "  didx: %d\n", didx);
    ck_assert_int_eq (didx, wkey);
  }

  danceselFree (ds);
  nlistFree (clist);
}
END_TEST

START_TEST(dancesel_choose_two_no_hist)
{
  dancesel_t  *ds;
  nlist_t     *clist = NULL;
  dance_t     *dances;
  slist_t     *dlist;
  ilistidx_t  wkey, tkey;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_two_no_hist");
// fprintf (stderr, "-- dancesel_choose_two_no_hist\n");

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");
  tkey = slistGetNum (dlist, "Tango");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  nlistSetNum (clist, wkey, 5);
  nlistSetNum (clist, tkey, 5);
  ds = danceselAlloc (clist);

  for (int i = 0; i < 16; ++i) {
    ilistidx_t  didx;
    int         rc;

    didx = danceselSelect (ds, clist, 0, NULL, NULL);
// fprintf (stderr, "  didx: %d\n", didx);
    rc = didx == wkey || didx == tkey;
    ck_assert_int_eq (rc, 1);
    /* without history, could be either waltz or tango */
  }

  danceselFree (ds);
  nlistFree (clist);
}
END_TEST

/* the following are checking against the queue, and not what was played */
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_two_hist_s");
// fprintf (stderr, "-- dancesel_choose_two_hist_s\n");

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");
  tkey = slistGetNum (dlist, "Tango");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  /* a symmetric outcome */
  nlistSetNum (clist, wkey, 5);
  nlistSetNum (clist, tkey, 5);
  ds = danceselAlloc (clist);

  lastdidx = -1;
  gprior = 0;
  for (int i = 0; i < 16; ++i) {
    ilistidx_t  didx;
    int         rc;

    didx = danceselSelect (ds, clist, gprior, chkQueue, NULL);
// fprintf (stderr, "  didx: %d\n", didx);
    rc = didx == wkey || didx == tkey;
    ck_assert_int_eq (rc, 1);
    /* with only two dances and the same counts, they should alternate */
    ck_assert_int_ne (didx, lastdidx);
    danceselAddCount (ds, didx);
    saveToQueue (didx);
    lastdidx = didx;
  }

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

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_two_hist_a");
// fprintf (stderr, "-- dancesel_choose_two_hist_a\n");

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
  ds = danceselAlloc (clist);

  gprior = 0;
  for (int i = 0; i < 12; ++i) {
    ilistidx_t  didx;
    int         rc;

    didx = danceselSelect (ds, clist, gprior, chkQueue, NULL);
// fprintf (stderr, "  didx: %d\n", didx);
    rc = didx == wkey || didx == tkey;
    ck_assert_int_eq (rc, 1);
    counts [didx]++;
    danceselAddCount (ds, didx);
    saveToQueue (didx);
  }
  /* with only two dances, the chances of a non-symmetric outcome */
  /* are low, but happen, so test a range */
  ck_assert_int_le (counts [wkey], 5);
  ck_assert_int_le (counts [tkey], 8);

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
  ilistidx_t  lastdidx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_multi_count");
// fprintf (stderr, "-- dancesel_choose_multi_count\n");

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
  ds = danceselAlloc (clist);

  lastdidx = -1;
  gprior = 0;
  for (int i = 0; i < 16; ++i) {
    ilistidx_t  didx;
    int         rc;

    didx = danceselSelect (ds, clist, gprior, chkQueue, NULL);
// fprintf (stderr, "  didx: %d\n", didx);
    rc = didx == wkey || didx == tkey || didx == rkey;
    ck_assert_int_eq (rc, 1);
    ck_assert_int_ne (didx, lastdidx);
    lastdidx = didx;
    counts [didx]++;
    danceselAddCount (ds, didx);
    saveToQueue (didx);
  }
  /* check for a range */
  ck_assert_int_le (counts [wkey], 5);
  ck_assert_int_le (counts [tkey], 5);
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

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_multi_tag");
// fprintf (stderr, "-- dancesel_choose_multi_tag\n");

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  fkey = slistGetNum (dlist, "Foxtrot");
  wkey = slistGetNum (dlist, "Waltz");
  jkey = slistGetNum (dlist, "Jive");
  wcskey = slistGetNum (dlist, "West Coast Swing");
  rkey = slistGetNum (dlist, "Rumba");

  clist = nlistAlloc ("count-list", LIST_ORDERED, NULL);
  nlistSetNum (clist, wkey, 8);
  nlistSetNum (clist, jkey, 8);
  nlistSetNum (clist, wcskey, 8);
  nlistSetNum (clist, rkey, 8);
  nlistSetNum (clist, fkey, 8);
  ds = danceselAlloc (clist);

  count = 0;
  lastdidx = -1;
  gprior = 0;
  for (int i = 0; i < 16; ++i) {
    ilistidx_t  didx;
    int         rc;

    didx = danceselSelect (ds, clist, gprior, chkQueue, NULL);
// fprintf (stderr, "  didx: %d\n", didx);
    rc = didx == wkey || didx == jkey || didx == wcskey ||
        didx == rkey || didx == fkey;
    ck_assert_int_eq (rc, 1);
    ck_assert_int_ne (didx, lastdidx);
    /* the tag match should help prevent a jive and a wcs */
    /* next to each other, but not always */
    if (didx == wcskey) {
      if (lastdidx == jkey) {
        ++count;
      }
    }
    if (didx == jkey) {
      if (lastdidx == wcskey) {
        ++count;
      }
    }
    lastdidx = didx;
    danceselAddCount (ds, didx);
    saveToQueue (didx);
  }
  /* there are too many variables to determine an exact value */
  /* but there should not be too many jive/wcs next to each other */
  ck_assert_int_lt (count, 4);

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

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- dancesel_choose_fast");
// fprintf (stderr, "-- dancesel_choose_fast\n");

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
  ds = danceselAlloc (clist);

  lastfast = 0;
  gprior = 0;
  for (int i = 0; i < 36; ++i) {
    ilistidx_t  didx;
    int         fast;
    int         notfast;

    didx = danceselSelect (ds, clist, gprior, chkQueue, NULL);
// fprintf (stderr, "  didx: %d\n", didx);
    fast = didx == jkey || didx == qskey;
    notfast = didx == wkey || didx == rkey || didx == fkey || didx == tkey;
    ck_assert_int_eq (fast | notfast, 1);
    if (i < 3) {
      ck_assert_int_eq (fast, 0);
      ck_assert_int_eq (notfast, 1);
    }
    if (i >= 3 && (lastfast || fast)) {
      /* fast dances never appear next to each other */
      ck_assert_int_ne (lastfast, fast);
    }
    lastfast = fast;
    danceselAddCount (ds, didx);
    saveToQueue (didx);
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
  tcase_add_test (tc, dancesel_choose_two_no_hist);
  tcase_add_test (tc, dancesel_choose_two_hist_s);
  tcase_add_test (tc, dancesel_choose_two_hist_a);
  tcase_add_test (tc, dancesel_choose_multi_count);
  tcase_add_test (tc, dancesel_choose_multi_tag);
  tcase_add_test (tc, dancesel_choose_fast);
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
