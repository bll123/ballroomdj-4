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

static char *dbfn = "data/musicdb.dat";

static void
setup (void)
{
  templateFileCopy ("dancetypes.txt", "dancetypes.txt");
  templateFileCopy ("dances.txt", "dances.txt");
  templateFileCopy ("genres.txt", "genres.txt");
  templateFileCopy ("levels.txt", "levels.txt");
  templateFileCopy ("ratings.txt", "ratings.txt");
  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");
}

START_TEST(samesong_alloc)
{
  musicdb_t     *db;
  samesong_t    *ss;

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);
  ss = samesongAlloc (db);
  samesongFree (ss);
  dbClose (db);
  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(samesong_basic)
{
  musicdb_t     *db;
  samesong_t    *ss;
  dbidx_t       dbidx;
  ssize_t       ssidx;
  song_t        *song;
  dbidx_t       dbiteridx;
  int           ucount;
  int           count;
  int           tcount;
  nlist_t       *tlist;
  nlistidx_t    iteridx;
  int           val;

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);
  ss = samesongAlloc (db);

  tlist = nlistAlloc ("chk-ss-basic", LIST_ORDERED, NULL);
  ucount = 0;
  count = 0;
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      ++count;
      val = nlistGetNum (tlist, ssidx);
      if (val < 0) {
        ++ucount;
        nlistSetNum (tlist, ssidx, 1);
      } else {
        val = nlistGetNum (tlist, ssidx);
        ++val;
        nlistSetNum (tlist, ssidx, val);
      }
    }
  }
  /* as of 2022-8-28, there are 3 same-song marks set in the test database */
  /* and each has two or more associated songs */
  ck_assert_int_eq (ucount, 3);
  ck_assert_int_ge (count, 7);

  tcount = 0;
  nlistStartIterator (tlist, &iteridx);
  while ((ssidx = nlistIterateKey (tlist, &iteridx)) > 0) {
    tcount += nlistGetNum (tlist, ssidx);
  }
  ck_assert_int_eq (count, tcount);

  samesongFree (ss);
  dbClose (db);
  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(samesong_get_by_ssidx)
{
  musicdb_t     *db;
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
  int           val;

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);
  ss = samesongAlloc (db);

  ucount = 0;
  tlist = nlistAlloc ("chk-ss-get-by-ss", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      val = nlistGetNum (tlist, ssidx);
      if (val < 0) {
        ++ucount;
        nlistSetNum (tlist, ssidx, 1);
      } else {
        val = nlistGetNum (tlist, ssidx);
        ++val;
        nlistSetNum (tlist, ssidx, val);
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
  dbClose (db);
  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(samesong_get_by_dbidx)
{
  musicdb_t     *db;
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
  int           val;

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);
  ss = samesongAlloc (db);

  ucount = 0;
  tlist = nlistAlloc ("chk-ss-get-by-db", LIST_ORDERED, NULL);
  dbidxlist = nlistAlloc ("chk-ss-get-by-db-db", LIST_ORDERED, NULL);
  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbidx, &dbiteridx)) != NULL) {
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      nlistSetNum (dbidxlist, dbidx, 1);
      val = nlistGetNum (tlist, ssidx);
      if (val < 0) {
        ++ucount;
        nlistSetNum (tlist, ssidx, 1);
      } else {
        val = nlistGetNum (tlist, ssidx);
        ++val;
        nlistSetNum (tlist, ssidx, val);
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
  dbClose (db);
  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(samesong_set)
{
  musicdb_t     *db;
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
  int           tucount;
  nlist_t       *tlist;
  nlist_t       *ndbidxlist;
  nlist_t       *dbidxlist;
  nlistidx_t    iteridx;
  slist_t       *clist;
  int           val;

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);
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
      val = nlistGetNum (tlist, ssidx);
      if (val < 0) {
        ++ucount;
        nlistSetNum (tlist, ssidx, 1);
        val = 1;
      } else {
        val = nlistGetNum (tlist, ssidx);
        ++val;
        nlistSetNum (tlist, ssidx, val);
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

  /* zero marks */
  /* choose 6-8 from the n-db-list.  */
  /* the first five have been marked already */
  /* this set is needed for the following test */
  tlist = nlistAlloc ("chk-ss-set-n67", LIST_ORDERED, NULL);
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

  /* at this point, the following sets from the n-db-list should have a mark */
  /* 1-4, 5-6, 7-8 */
  tlist = nlistAlloc ("chk-ss-set-c", LIST_ORDERED, NULL);
  tucount = 0;
  tcount = 0;
  nlistStartIterator (ndbidxlist, &iteridx);
  while ((dbidx = nlistIterateKey (ndbidxlist, &iteridx)) >= 0) {
    song = dbGetByIdx (db, dbidx);
    ssidx = songGetNum (song, TAG_SAMESONG);
    if (ssidx > 0) {
      ++tcount;
      val = nlistGetNum (tlist, ssidx);
      if (val < 0) {
        ++tucount;
        nlistSetNum (tlist, ssidx, 1);
      } else {
        val = nlistGetNum (tlist, ssidx);
        ++val;
        nlistSetNum (tlist, ssidx, val);
      }
    }
    if (tcount > 7) {
      break;
    }
  }
  /* from among the first 8 songs in the n-db-list */
  ck_assert_int_eq (ucount, 3);
  ck_assert_int_eq (tcount, 8);
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
  ck_assert_int_eq (ucount + 3, tcount);
  ck_assert_int_eq (ucount + 3, slistGetCount (clist));

  samesongFree (ss);
  dbClose (db);
  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

Suite *
samesong_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("samesong");
  tc = tcase_create ("samesong");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_set_tags (tc, "libbdj4");
  tcase_add_test (tc, samesong_alloc);
  tcase_add_test (tc, samesong_basic);
  tcase_add_test (tc, samesong_get_by_ssidx);
  tcase_add_test (tc, samesong_get_by_dbidx);
  tcase_add_test (tc, samesong_set);
  suite_add_tcase (s, tc);
  return s;
}

