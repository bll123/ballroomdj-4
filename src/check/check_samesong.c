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
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "filemanip.h"
#include "musicdb.h"
#include "nlist.h"
#include "samesong.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"
#include "templateutil.h"

static bool updateSSList (nlist_t *tlist, ssize_t ssidx);
static void ssListGetCounts (nlist_t *tlist, int *ucount, int *totcount);

static char *dbfn = "data/musicdb.dat";
static musicdb_t *db = NULL;

static void
setup (void)
{
  templateFileCopy ("dancetypes.txt", "dancetypes.txt");
  templateFileCopy ("dances.txt", "dances.txt");
  templateFileCopy ("genres.txt", "genres.txt");
  templateFileCopy ("levels.txt", "levels.txt");
  templateFileCopy ("ratings.txt", "ratings.txt");
  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
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

START_TEST(samesong_alloc)
{
  samesong_t    *ss;

  ss = samesongAlloc (db);
  samesongFree (ss);
}
END_TEST

START_TEST(samesong_basic)
{
  samesong_t    *ss;
  dbidx_t       dbidx;
  ssize_t       ssidx;
  song_t        *song;
  dbidx_t       dbiteridx;
  int           ucount;
  int           count;
  int           tcount;
  nlist_t       *tlist;
  int           val;

  ss = samesongAlloc (db);

  tlist = nlistAlloc ("chk-ss-basic", LIST_ORDERED, NULL);
  ucount = 0;
  count = 0;
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      ++count;
      if (updateSSList (tlist, ssidx)) {
        ++ucount;
      }
    }
  }
  /* as of 2022-8-28, there are 3 same-song marks set in the test database */
  /* and each has two or more associated songs */
  ck_assert_int_eq (ucount, 3);
  ck_assert_int_ge (count, 7);

  ssListGetCounts (tlist, &tcount, &val);
  ck_assert_int_eq (ucount, tcount);
  ck_assert_int_eq (count, val);

  samesongFree (ss);
}
END_TEST

START_TEST(samesong_get_by_ssidx)
{
  samesong_t    *ss;
  dbidx_t       dbidx;
  ssize_t       ssidx;
  song_t        *song;
  dbidx_t       dbiteridx;
  int           ucount;
  int           tcount;
  nlist_t       *tlist;
  nlistidx_t    iteridx;
  slist_t       *clist;

  ss = samesongAlloc (db);

  ucount = 0;
  tlist = nlistAlloc ("chk-ss-get-by-ss", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      if (updateSSList (tlist, ssidx)) {
        ++ucount;
      }
    }
  }

  /* check to make sure the colors are unique */
  /* it is possible that this could fail, but unlikely */
  tcount = 0;
  clist = slistAlloc ("chk-ss-get-by-ss-col", LIST_ORDERED, NULL);
  nlistStartIterator (tlist, &iteridx);
  while ((ssidx = nlistIterateKey (tlist, &iteridx)) > 0) {
    const char *sscolor;

    sscolor = samesongGetColorBySSIdx (ss, ssidx);
    if (slistGetNum (clist, sscolor) < 0) {
      ++tcount;
      slistSetNum (clist, sscolor, 1);
    }
  }
  ck_assert_int_eq (ucount, tcount);
  ck_assert_int_eq (ucount, slistGetCount (clist));

  samesongFree (ss);
}
END_TEST

START_TEST(samesong_get_by_dbidx)
{
  samesong_t    *ss;
  dbidx_t       dbidx;
  song_t        *song;
  ssize_t       ssidx;
  dbidx_t       dbiteridx;
  int           ucount;
  int           tcount;
  nlist_t       *tlist;
  nlist_t       *dbidxlist;
  nlistidx_t    iteridx;
  slist_t       *clist;

  ss = samesongAlloc (db);

  ucount = 0;
  tlist = nlistAlloc ("chk-ss-get-by-db", LIST_ORDERED, NULL);
  dbidxlist = nlistAlloc ("chk-ss-get-by-db-db", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      nlistSetNum (dbidxlist, dbidx, 1);
      if (updateSSList (tlist, ssidx)) {
        ++ucount;
      }
    }
  }

  /* check to make sure the colors are unique */
  /* it is possible that this could fail, but unlikely */
  tcount = 0;
  clist = slistAlloc ("chk-ss-get-by-db-col", LIST_ORDERED, NULL);
  nlistStartIterator (dbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (dbidxlist, &iteridx)) >= 0) {
    const char *sscolor;
    const char *dsscolor;

    song = dbGetByIdx (db, dbidx);
    ssidx = songGetNum (song, TAG_SAMESONG);
    sscolor = samesongGetColorBySSIdx (ss, ssidx);
    dsscolor = samesongGetColorByDBIdx (ss, dbidx);
    ck_assert_str_eq (sscolor, dsscolor);
    if (slistGetNum (clist, dsscolor) < 0) {
      ++tcount;
      slistSetNum (clist, dsscolor, 1);
    }
  }
  ck_assert_int_eq (ucount, tcount);
  ck_assert_int_eq (ucount, slistGetCount (clist));

  samesongFree (ss);
}
END_TEST

/* this test should not exercise the singleton checks */
/* a separate test is used */
START_TEST(samesong_set)
{
  samesong_t    *ss;
  dbidx_t       dbidx;
  song_t        *song;
  ssize_t       ssidx;
  ssize_t       lastssidx;
  ssize_t       maxssidx;
  const char    *lastsscolor;
  const char    *sscolor;
  dbidx_t       dbiteridx;
  int           ucount;
  int           totcount;
  int           tcount;
  nlist_t       *tlist;
  nlist_t       *ndbidxlist;
  nlist_t       *dbidxlist;
  nlistidx_t    iteridx;
  slist_t       *clist;

  ss = samesongAlloc (db);

  ucount = 0;
  totcount = 0;
  maxssidx = 0;
  tlist = nlistAlloc ("chk-ss-set-a", LIST_ORDERED, NULL);
  dbidxlist = nlistAlloc ("chk-ss-set-db", LIST_ORDERED, NULL);
  ndbidxlist = nlistAlloc ("chk-ss-set-ndb", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      if (ssidx > maxssidx) {
        maxssidx = ssidx;
      }
      ++totcount;
      nlistSetNum (dbidxlist, dbidx, 1);
      if (updateSSList (tlist, ssidx)) {
        ++ucount;
      }
    } else {
      nlistSetNum (ndbidxlist, dbidx, 1);
    }
  }
  nlistFree (tlist);

  /* zero marks, choose the first 3 from the n-db-list */
  tlist = nlistAlloc ("chk-ss-set-n3", LIST_ORDERED, NULL);
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    nlistSetNum (tlist, dbidx, 0);
    ++tcount;
    if (tcount > 2) {
      break;
    }
  }
  ck_assert_int_eq (nlistGetCount (tlist), 3);

  samesongSet (ss, tlist);

  lastssidx = -1;
  lastsscolor = NULL;
  nlistStartIterator (tlist, &iteridx);
  while ((dbidx = nlistIterateKey (tlist, &iteridx)) >= 0) {
    song = dbGetByIdx (db, dbidx);
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (lastssidx == -1) {
      lastssidx = ssidx;
      lastsscolor = samesongGetColorBySSIdx (ss, ssidx);
    }
    /* should be set */
    ck_assert_int_gt (ssidx, 0);
    /* with zero marks, the ss-idx should be new and largest */
    ck_assert_int_gt (ssidx, maxssidx);
    /* should all have the same idx */
    ck_assert_int_eq (ssidx, lastssidx);
    /* and the same color */
    sscolor = samesongGetColorBySSIdx (ss, ssidx);
    ck_assert_str_eq (sscolor, lastsscolor);
  }
  maxssidx = lastssidx;
  nlistFree (tlist);

  /* one mark */
  /* choose the first 4 from the n-db-list.  */
  /* the first three have been marked already */
  tlist = nlistAlloc ("chk-ss-set-n4", LIST_ORDERED, NULL);
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    nlistSetNum (tlist, dbidx, 0);
    ++tcount;
    if (tcount > 3) {
      break;
    }
  }
  ck_assert_int_eq (nlistGetCount (tlist), 4);

  samesongSet (ss, tlist);

  lastsscolor = NULL;
  nlistStartIterator (tlist, &iteridx);
  while ((dbidx = nlistIterateKey (tlist, &iteridx)) > 0) {
    song = dbGetByIdx (db, dbidx);
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (lastsscolor == NULL) {
      lastsscolor = samesongGetColorBySSIdx (ss, ssidx);
    }
    /* should be set */
    ck_assert_int_gt (ssidx, 0);
    /* with one mark, the ss-idx should be the same as the first 3 */
    ck_assert_int_eq (ssidx, lastssidx);
    /* and the same color */
    sscolor = samesongGetColorBySSIdx (ss, ssidx);
    ck_assert_str_eq (sscolor, lastsscolor);
  }
  nlistFree (tlist);

  /* one mark */
  /* choose 4-5 from the n-db-list.  */
  /* the first one has been marked already */
  tlist = nlistAlloc ("chk-ss-set-n45", LIST_ORDERED, NULL);
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    ++tcount;
    if (tcount > 3) {
      nlistSetNum (tlist, dbidx, 0);
    }
    if (tcount > 4) {
      break;
    }
  }
  ck_assert_int_eq (nlistGetCount (tlist), 2);

  samesongSet (ss, tlist);

  lastsscolor = NULL;
  nlistStartIterator (tlist, &iteridx);
  while ((dbidx = nlistIterateKey (tlist, &iteridx)) > 0) {
    song = dbGetByIdx (db, dbidx);
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (lastsscolor == NULL) {
      lastsscolor = samesongGetColorBySSIdx (ss, ssidx);
    }
    /* should be set */
    ck_assert_int_gt (ssidx, 0);
    /* with one mark, the ss-idx should be the same as the first 3 */
    ck_assert_int_eq (ssidx, lastssidx);
    /* and the same color */
    sscolor = samesongGetColorBySSIdx (ss, ssidx);
    ck_assert_str_eq (sscolor, lastsscolor);
  }
  nlistFree (tlist);

  /* now iterate through the entire db and make sure the counts are correct */
  tcount = 0;
  tlist = nlistAlloc ("chk-ss-set-chk-db-a", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      nlistSetNum (tlist, ssidx, 1);
      ++tcount;
    }
  }
  ck_assert_int_eq (totcount + 5, tcount);
  ck_assert_int_eq (ucount + 1, nlistGetCount (tlist));
  nlistFree (tlist);

  /* zero marks */
  /* choose 6-8 from the n-db-list.  */
  /* the first five have been marked already */
  /* this set is needed for the following test */
  tlist = nlistAlloc ("chk-ss-set-n678", LIST_ORDERED, NULL);
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    ++tcount;
    if (tcount > 5) {
      nlistSetNum (tlist, dbidx, 0);
    }
    if (tcount > 7) {
      break;
    }
  }
  ck_assert_int_eq (nlistGetCount (tlist), 3);

  samesongSet (ss, tlist);

  lastssidx = -1;
  lastsscolor = NULL;
  nlistStartIterator (tlist, &iteridx);
  while ((dbidx = nlistIterateKey (tlist, &iteridx)) > 0) {
    song = dbGetByIdx (db, dbidx);
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (lastssidx == -1) {
      lastssidx = ssidx;
      lastsscolor = samesongGetColorBySSIdx (ss, ssidx);
    }
    /* should be set */
    ck_assert_int_gt (ssidx, 0);
    /* with zero marks, the ss-idx should be new and largest */
    ck_assert_int_gt (ssidx, maxssidx);
    /* should all have the same idx */
    ck_assert_int_eq (ssidx, lastssidx);
    /* and the same color */
    sscolor = samesongGetColorBySSIdx (ss, ssidx);
    ck_assert_str_eq (sscolor, lastsscolor);
  }
  maxssidx = lastssidx;
  nlistFree (tlist);

  /* now iterate through the entire db and make sure the counts are correct */
  tcount = 0;
  tlist = nlistAlloc ("chk-ss-set-chk-db-b", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      updateSSList (tlist, ssidx);
      ++tcount;
    }
  }
  ck_assert_int_eq (totcount + 8, tcount);
  ck_assert_int_eq (ucount + 2, nlistGetCount (tlist));
  nlistFree (tlist);

  /* many marks */
  /* choose 5-6 from the n-db-list.  */
  /* the first five have a mark */
  /* the next three have a mark */
  tlist = nlistAlloc ("chk-ss-set-n56", LIST_ORDERED, NULL);
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    ++tcount;
    if (tcount > 4) {
      nlistSetNum (tlist, dbidx, 0);
    }
    if (tcount > 5) {
      break;
    }
  }

  samesongSet (ss, tlist);

  lastssidx = -1;
  lastsscolor = NULL;
  nlistStartIterator (tlist, &iteridx);
  while ((dbidx = nlistIterateKey (tlist, &iteridx)) > 0) {
    song = dbGetByIdx (db, dbidx);
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (lastssidx == -1) {
      lastssidx = ssidx;
      lastsscolor = samesongGetColorBySSIdx (ss, ssidx);
    }
    /* should be set */
    ck_assert_int_gt (ssidx, 0);
    /* with many marks, the ss-idx should be new and largest */
    ck_assert_int_gt (ssidx, maxssidx);
    /* should all have the same idx */
    ck_assert_int_eq (ssidx, lastssidx);
    /* and the same color */
    sscolor = samesongGetColorBySSIdx (ss, ssidx);
    ck_assert_str_eq (sscolor, lastsscolor);
  }
  maxssidx = lastssidx;
  nlistFree (tlist);

  /* now iterate through the entire db and make sure the counts are correct */
  tcount = 0;
  tlist = nlistAlloc ("chk-ss-set-chk-db-b", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      nlistSetNum (tlist, ssidx, 1);
      ++tcount;
    }
  }
  ck_assert_int_eq (totcount + 8, tcount);
  ck_assert_int_eq (ucount + 3, nlistGetCount (tlist));
  nlistFree (tlist);

  /* one mark, no unset */
  /* choose the first two in the n-db-list */
  tlist = nlistAlloc ("chk-ss-set-n2", LIST_ORDERED, NULL);
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    nlistSetNum (tlist, dbidx, 0);
    ++tcount;
    if (tcount > 1) {
      break;
    }
  }
  ck_assert_int_eq (nlistGetCount (tlist), 2);

  samesongSet (ss, tlist);

  lastssidx = -1;
  lastsscolor = NULL;
  nlistStartIterator (tlist, &iteridx);
  while ((dbidx = nlistIterateKey (tlist, &iteridx)) >= 0) {
    song = dbGetByIdx (db, dbidx);
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (lastssidx == -1) {
      lastssidx = ssidx;
      lastsscolor = samesongGetColorBySSIdx (ss, ssidx);
    }
    /* should be set */
    ck_assert_int_gt (ssidx, 0);
    /* with one mark, no unset, the ss-idx should be new and largest */
    ck_assert_int_gt (ssidx, maxssidx);
    /* should all have the same idx */
    ck_assert_int_eq (ssidx, lastssidx);
    /* and the same color */
    sscolor = samesongGetColorBySSIdx (ss, ssidx);
    ck_assert_str_eq (sscolor, lastsscolor);
  }
  maxssidx = lastssidx;
  nlistFree (tlist);

  /* now iterate through the entire db and make sure the counts are correct */
  tcount = 0;
  tlist = nlistAlloc ("chk-ss-set-chk-db-b", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      nlistSetNum (tlist, ssidx, 1);
      ++tcount;
    }
  }
  ck_assert_int_eq (totcount + 8, tcount);
  ck_assert_int_eq (ucount + 4, nlistGetCount (tlist));
  nlistFree (tlist);

  /* check to make sure the colors are unique */
  /* it is possible that this could fail, but unlikely */
  /* iterate through the database this time to catch the originals from */
  /* the db load in addition to the songs that have been set */
  tcount = 0;
  clist = slistAlloc ("chk-set-d-col", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    const char *sscolor = NULL;

    sscolor = samesongGetColorByDBIdx (ss, dbidx);
    if (sscolor != NULL) {
      if (slistGetNum (clist, sscolor) < 0) {
        ++tcount;
        slistSetNum (clist, sscolor, 1);
      }
    }
  }
  ck_assert_int_eq (ucount + 4, tcount);
  ck_assert_int_eq (ucount + 4, slistGetCount (clist));

  samesongFree (ss);
}
END_TEST

START_TEST(samesong_singleton)
{
  samesong_t    *ss;
  dbidx_t       dbidx;
  dbidx_t       dbidxa;
  dbidx_t       dbidxb;
  dbidx_t       dbidxc;
  song_t        *song;
  ssize_t       ssidx;
  const char    *sscolor;
  dbidx_t       dbiteridx;
  nlist_t       *tlist;
  int           tcount;
  nlist_t       *ndbidxlist;
  nlist_t       *dbidxlist;
  nlistidx_t    iteridx;

  ss = samesongAlloc (db);

  dbidxa = -1;
  dbidxb = -1;
  dbidxc = -1;
  tlist = nlistAlloc ("chk-ss-set-a", LIST_ORDERED, NULL);
  dbidxlist = nlistAlloc ("chk-ss-set-db", LIST_ORDERED, NULL);
  ndbidxlist = nlistAlloc ("chk-ss-set-ndb", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      nlistSetNum (dbidxlist, dbidx, 1);
      updateSSList (tlist, ssidx);
    } else {
      nlistSetNum (ndbidxlist, dbidx, 1);
    }
  }
  nlistFree (tlist);

  /* choose 1-3 from the n-db-list.  */
  tlist = nlistAlloc ("chk-ss-s-set-n13", LIST_ORDERED, NULL);
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    ++tcount;
    nlistSetNum (tlist, dbidx, 0);
    if (tcount == 1) {
      dbidxa = dbidx;
    }
    if (tcount == 2) {
      dbidxb = dbidx;
    }
    if (tcount == 3) {
      dbidxc = dbidx;
    }
    if (tcount > 2) {
      break;
    }
  }

  samesongSet (ss, tlist);
  nlistFree (tlist);

  sscolor = samesongGetColorByDBIdx (ss, dbidxa);
  ck_assert_ptr_nonnull (sscolor);
  sscolor = samesongGetColorByDBIdx (ss, dbidxb);
  ck_assert_ptr_nonnull (sscolor);

  /* choose 2-3 from the n-db-list.  */
  /* this will leave 1 as a singleton */
  tlist = nlistAlloc ("chk-ss-s-set-n12", LIST_ORDERED, NULL);
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    ++tcount;
    if (tcount > 1) {
      nlistSetNum (tlist, dbidx, 0);
    }
    if (tcount > 2) {
      break;
    }
  }

  samesongSet (ss, tlist);
  nlistFree (tlist);

  sscolor = samesongGetColorByDBIdx (ss, dbidxa);
  ck_assert_ptr_null (sscolor);
  sscolor = samesongGetColorByDBIdx (ss, dbidxb);
  ck_assert_ptr_nonnull (sscolor);
  sscolor = samesongGetColorByDBIdx (ss, dbidxc);
  ck_assert_ptr_nonnull (sscolor);

  samesongFree (ss);
}
END_TEST

START_TEST(samesong_clear)
{
  samesong_t    *ss;
  dbidx_t       dbidx;
  song_t        *song;
  ssize_t       ssidx;
  int           ucount;
  int           totcount;
  int           tcount;
  dbidx_t       dbiteridx;
  nlist_t       *tlist;
  nlist_t       *ndbidxlist;
  nlist_t       *dbidxlist;
  nlistidx_t    iteridx;

  ss = samesongAlloc (db);

  ucount = 0;
  totcount = 0;
  tlist = nlistAlloc ("chk-ss-set-a", LIST_ORDERED, NULL);
  dbidxlist = nlistAlloc ("chk-ss-set-db", LIST_ORDERED, NULL);
  ndbidxlist = nlistAlloc ("chk-ss-set-ndb", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      ++totcount;
      nlistSetNum (dbidxlist, dbidx, 1);
      if (updateSSList (tlist, ssidx)) {
        ++ucount;
      }
    } else {
      nlistSetNum (ndbidxlist, dbidx, 1);
    }
  }
  nlistFree (tlist);

  /* zero marks, choose the first 3 from the n-db-list */
  tlist = nlistAlloc ("chk-ss-set-n13", LIST_ORDERED, NULL);
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    ++tcount;
    nlistSetNum (tlist, dbidx, 0);
    if (tcount > 2) {
      break;
    }
  }
  ck_assert_int_eq (nlistGetCount (tlist), 3);
  samesongSet (ss, tlist);
  nlistFree (tlist);

  /* zero marks, choose the second 3 from the n-db-list */
  tlist = nlistAlloc ("chk-ss-set-n46", LIST_ORDERED, NULL);
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    ++tcount;
    if (tcount > 3) {
      nlistSetNum (tlist, dbidx, 0);
    }
    if (tcount > 5) {
      break;
    }
  }
  ck_assert_int_eq (nlistGetCount (tlist), 3);
  samesongSet (ss, tlist);
  nlistFree (tlist);

  tlist = nlistAlloc ("chk-ss-set-n23", LIST_ORDERED, NULL);
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    ++tcount;
    if (tcount > 2) {
      nlistSetNum (tlist, dbidx, 0);
    }
    if (tcount > 3) {
      break;
    }
  }
  ck_assert_int_eq (nlistGetCount (tlist), 2);
  samesongClear (ss, tlist);

  nlistStartIterator (tlist, &iteridx);
  while ((dbidx = nlistIterateKey (tlist, &iteridx)) >= 0) {
    const char *sscolor;

    sscolor = samesongGetColorByDBIdx (ss, dbidx);
    ck_assert_ptr_null (sscolor);
  }

  nlistFree (tlist);

  /* now iterate through the entire db and make sure the counts are correct */
  tcount = 0;
  tlist = nlistAlloc ("chk-ss-set-chk-db-b", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      nlistSetNum (tlist, ssidx, 1);
      ++tcount;
    }
  }
  ck_assert_int_eq (totcount + 4, tcount);
  ck_assert_int_eq (ucount + 2, nlistGetCount (tlist));
  nlistFree (tlist);
}
END_TEST

START_TEST(ss_check_alloc)
{
}
END_TEST

Suite *
samesong_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("samesong");
  tc = tcase_create ("samesong");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_set_tags (tc, "libbdj4");
  tcase_add_test (tc, samesong_alloc);
  tcase_add_test (tc, samesong_basic);
  tcase_add_test (tc, samesong_get_by_ssidx);
  tcase_add_test (tc, samesong_get_by_dbidx);
  tcase_add_test (tc, samesong_set);
  tcase_add_test (tc, samesong_singleton);
  tcase_add_test (tc, samesong_clear);
  suite_add_tcase (s, tc);

  tc = tcase_create ("samesong-check");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, ss_check_alloc);
  suite_add_tcase (s, tc);

  return s;
}

static bool
updateSSList (nlist_t *tlist, ssize_t ssidx)
{
  int   val;
  bool  u = false;

  val = nlistGetNum (tlist, ssidx);
  if (val < 0) {
    u = true;
    val = 1;
  } else {
    val = nlistGetNum (tlist, ssidx);
    ++val;
  }
  nlistSetNum (tlist, ssidx, val);
  return u;
}

static void
ssListGetCounts (nlist_t *tlist, int *ucount, int *totcount)
{
  nlistidx_t  iteridx;
  ssize_t     ssidx;

  *totcount = 0;
  *ucount = 0;
  nlistStartIterator (tlist, &iteridx);
  while ((ssidx = nlistIterateKey (tlist, &iteridx)) > 0) {
    ++(*ucount);
    *totcount += nlistGetNum (tlist, ssidx);
  }
}
