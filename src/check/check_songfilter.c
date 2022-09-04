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
#include "filemanip.h"
#include "genre.h"
#include "level.h"
#include "log.h"
#include "musicdb.h"
#include "rating.h"
#include "songfilter.h"
#include "status.h"
#include "templateutil.h"
#include "tmutil.h"

/* all the standard sort types from sortopt.txt */
static char *sorttypes [] = {
  "ALBUMARTIST TITLE",
  "ALBUMARTIST ALBUM TRACKNUMBER",
  "ALBUM TRACKNUMBER",
  "ARTIST TITLE",
  "DANCE TITLE",
  "DANCE DANCELEVEL TITLE",
  "DANCE DANCERATING TITLE",
  "DANCELEVEL DANCE TITLE",
  "DANCELEVEL TITLE",
  "DBADDDATE",
  "GENRE ALBUMARTIST ALBUM TRACKNUMBER",
  "GENRE ALBUMARTIST TITLE",
  "GENRE ALBUM TRACKNUMBER",
  "TITLE",
  "LASTUPDATE",
};
enum {
  sorttypesz = sizeof (sorttypes) / sizeof (char *),
};

static char *dbfn = "data/musicdb.dat";
static musicdb_t  *db = NULL;

static void
setup (void)
{
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

START_TEST(songfilter_alloc)
{
  songfilter_t  *sf;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_alloc");

  sf = songfilterAlloc ();
  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_set_sort)
{
  songfilter_t  *sf;
  const char    *p;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_set_sort");

  sf = songfilterAlloc ();
  for (int i = 0; i < sorttypesz; ++i) {
    songfilterSetSort (sf, sorttypes [i]);
    p = songfilterGetSort (sf);
    ck_assert_str_eq (sorttypes [i], p);
  }
  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_clear)
{
  songfilter_t  *sf;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_clear");

  sf = songfilterAlloc ();
  for (int i = 0; i < SONG_FILTER_MAX; ++i) {
    songfilterClear (sf, i);
  }
  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_reset)
{
  songfilter_t  *sf;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_reset");

  sf = songfilterAlloc ();
  songfilterReset (sf);
  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_changed)
{
  songfilter_t  *sf;
  time_t        ctm;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_changed");

  sf = songfilterAlloc ();
  ctm = mstime ();
  ck_assert_int_eq (songfilterIsChanged (sf, ctm), 0);
  mssleep (10);
  songfilterSetSort (sf, sorttypes [0]);
  ck_assert_int_eq (songfilterIsChanged (sf, ctm), 1);
  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_process)
{
  songfilter_t  *sf;
  dbidx_t       rv, tv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_process");

  sf = songfilterAlloc ();
  rv = songfilterProcess (sf, db);
  ck_assert_int_gt (rv, 0);
  tv = songfilterGetCount (sf);
  ck_assert_int_eq (rv, tv);
  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_rating)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv, temprv;
  rating_t      *ratings;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_rating");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);
  temprv = 0;

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 0);

  for (int i = ratingGetCount (ratings) - 1; i >= 0; --i) {
    songfilterSetNum (sf, SONG_FILTER_RATING, i);

    rv = songfilterProcess (sf, db);
    ck_assert_int_le (rv, arv);
    /* a rating filter includes all the higher ratings, so as the rating list */
    /* is traversed downwards, the number returned must go up (or be equal) */
    ck_assert_int_ge (rv, temprv);
    temprv = rv;
  }

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 1);

  /* the first rating, unrated, should return all selections */
  ck_assert_int_eq (temprv, arv);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_off_on)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_off_on");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 0);
  songfilterSetNum (sf, SONG_FILTER_RATING, 3); // good
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 1);

  /* make sure some other filters are off */
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_GENRE), 0);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_FAVORITE), 0);

  /* turn off, make sure off works */
  songfilterOff (sf, SONG_FILTER_RATING);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 0);
  songfilterOn (sf, SONG_FILTER_RATING);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 1);
  songfilterOff (sf, SONG_FILTER_RATING);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 0);

  rv = songfilterProcess (sf, db);
  ck_assert_int_eq (rv, arv);

  songfilterOn (sf, SONG_FILTER_RATING);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 1);
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_genre)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv, trv;
  genre_t       *genres;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_genre");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);
  trv = 0;

  genres = bdjvarsdfGet (BDJVDF_GENRES);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_GENRE), 0);

  for (int i = 0; i < genreGetCount (genres); ++i) {
    songfilterSetNum (sf, SONG_FILTER_GENRE, i);

    rv = songfilterProcess (sf, db);
    ck_assert_int_lt (rv, arv);
    trv += rv;
  }

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_GENRE), 1);
  ck_assert_int_le (trv, arv);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_level)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv, trv;
  dbidx_t       temprv [5];
  level_t       *levels;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_level");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);
  trv = 0;

  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_LEVEL_LOW), 0);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_LEVEL_HIGH), 0);

  for (int i = 0; i < levelGetCount (levels); ++i) {
    temprv [i] = 0;
    songfilterSetNum (sf, SONG_FILTER_LEVEL_LOW, i);
    songfilterSetNum (sf, SONG_FILTER_LEVEL_HIGH, i);

    rv = songfilterProcess (sf, db);
    ck_assert_int_lt (rv, arv);
    temprv [i] = rv;
    trv += rv;
  }

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_LEVEL_LOW), 1);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_LEVEL_HIGH), 1);
  ck_assert_int_eq (trv, arv);

  for (int i = 0; i < levelGetCount (levels) - 1; ++i) {
    songfilterSetNum (sf, SONG_FILTER_LEVEL_LOW, i);
    songfilterSetNum (sf, SONG_FILTER_LEVEL_HIGH, i + 1);
    rv = songfilterProcess (sf, db);
    ck_assert_int_lt (rv, arv);
    ck_assert_int_eq (rv, temprv [i] + temprv [i + 1]);
  }

  for (int i = 0; i < levelGetCount (levels) - 2; ++i) {
    songfilterSetNum (sf, SONG_FILTER_LEVEL_LOW, i);
    songfilterSetNum (sf, SONG_FILTER_LEVEL_HIGH, i + 2);
    rv = songfilterProcess (sf, db);
    ck_assert_int_le (rv, arv);
    ck_assert_int_eq (rv, temprv [i] + temprv [i + 1] + temprv [i + 2]);
  }

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_status)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv, trv;
  status_t      *status;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_status");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);
  trv = 0;

  status = bdjvarsdfGet (BDJVDF_STATUS);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_STATUS), 0);

  for (int i = 0; i < statusGetCount (status); ++i) {
    songfilterSetNum (sf, SONG_FILTER_STATUS, i);

    rv = songfilterProcess (sf, db);
    ck_assert_int_lt (rv, arv);
    trv += rv;
  }

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_STATUS), 1);
  ck_assert_int_eq (trv, arv);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_playable)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_playable");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);

  /* the status playable flag does not receive a value, it is simply */
  /* turned on or off */
  /* if off, no check is done */
  songfilterOff (sf, SONG_FILTER_STATUS_PLAYABLE);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_STATUS_PLAYABLE), 0);
  rv = songfilterProcess (sf, db);
  ck_assert_int_eq (rv, arv);

  songfilterOn (sf, SONG_FILTER_STATUS_PLAYABLE);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_STATUS_PLAYABLE), 1);
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_keyword)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv, rva, rvb;
  slist_t       *lista;
  slist_t       *listb;
  slist_t       *listall;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_keyword");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);

  lista = slistAlloc ("chk-sf-lista", LIST_ORDERED, NULL);
  slistSetNum (lista, "aaa", 0);
  listb = slistAlloc ("chk-sf-listb", LIST_ORDERED, NULL);
  slistSetNum (listb, "bbb", 0);
  listall = slistAlloc ("chk-sf-listall", LIST_ORDERED, NULL);
  slistSetNum (listall, "aaa", 0);
  slistSetNum (listall, "bbb", 0);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_KEYWORD), 0);

  songfilterSetData (sf, SONG_FILTER_KEYWORD, lista);
  rv = songfilterProcess (sf, db);
  ck_assert_int_gt (rv, arv);
  ck_assert_int_gt (rv, 0);
  rva = rv - arv;

  songfilterSetData (sf, SONG_FILTER_KEYWORD, listb);
  rv = songfilterProcess (sf, db);
  ck_assert_int_gt (rv, arv);
  ck_assert_int_gt (rv, 0);
  rvb = rv - arv;

  songfilterSetData (sf, SONG_FILTER_KEYWORD, listall);
  rv = songfilterProcess (sf, db);
  ck_assert_int_gt (rv, arv);
  ck_assert_int_gt (rv, 0);
  ck_assert_int_eq (rv, rva + rvb + arv);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_KEYWORD), 1);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_dance_idx)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv;
  dance_t       *dances;
  slist_t       *dlist;
  ilistidx_t    wkey;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_dance_idx");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_DANCE_IDX), 0);
  songfilterSetNum (sf, SONG_FILTER_DANCE_IDX, wkey);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_DANCE_IDX), 1);

  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_dance_list)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv, rvw, rvt;
  dance_t       *dances;
  ilist_t       *dflist;
  slist_t       *dlist;
  ilistidx_t    wkey, tkey;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_dance_list");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");
  tkey = slistGetNum (dlist, "Tango");

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_DANCE_LIST), 0);

  dflist = ilistAlloc ("chk-sf-dnc-list", LIST_ORDERED);
  ilistSetNum (dflist, wkey, 0, 0);
  songfilterSetData (sf, SONG_FILTER_DANCE_LIST, dflist);
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);
  rvw = rv;

  dflist = ilistAlloc ("chk-sf-dnc-list", LIST_ORDERED);
  ilistSetNum (dflist, tkey, 0, 0);
  songfilterSetData (sf, SONG_FILTER_DANCE_LIST, dflist);
  rv = songfilterProcess (sf, db);
  rvt = rv;
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);

  ilistSetNum (dflist, wkey, 0, 0);
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);
  ck_assert_int_eq (rv, rvw + rvt);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_DANCE_LIST), 1);

  songfilterFree (sf);
}
END_TEST

Suite *
songfilter_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("songfilter");
  tc = tcase_create ("songfilter");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, songfilter_alloc);
  tcase_add_test (tc, songfilter_set_sort);
  tcase_add_test (tc, songfilter_clear);
  tcase_add_test (tc, songfilter_reset);
  tcase_add_test (tc, songfilter_changed);
  tcase_add_test (tc, songfilter_process);
  tcase_add_test (tc, songfilter_rating);
  tcase_add_test (tc, songfilter_off_on);
  tcase_add_test (tc, songfilter_genre);
  tcase_add_test (tc, songfilter_level);
  tcase_add_test (tc, songfilter_status);
  tcase_add_test (tc, songfilter_playable);
  tcase_add_test (tc, songfilter_keyword);
  tcase_add_test (tc, songfilter_dance_idx);
  tcase_add_test (tc, songfilter_dance_list);
  suite_add_tcase (s, tc);
  return s;
}

