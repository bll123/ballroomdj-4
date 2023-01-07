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
#include "songfav.h"
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

START_TEST(songfilter_clear)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_clear");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 0);
  songfilterSetNum (sf, SONG_FILTER_RATING, 3); // good
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 1);

  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);

  songfilterClear (sf, SONG_FILTER_RATING);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 0);
  rv = songfilterProcess (sf, db);
  ck_assert_int_eq (rv, arv);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_reset)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_reset");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 0);
  songfilterSetNum (sf, SONG_FILTER_RATING, 3); // good
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 1);

  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);

  songfilterReset (sf);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 0);

  rv = songfilterProcess (sf, db);
  ck_assert_int_eq (rv, arv);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_get_byidx)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_get_byidx");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 0);
  songfilterSetNum (sf, SONG_FILTER_RATING, 3); // good
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 1);

  rv = songfilterGetCount (sf);
  ck_assert_int_eq (rv, arv);
  for (int i = 0; i < rv; ++i) {
    dbidx_t dbidx;
    song_t  *song;

    dbidx = songfilterGetByIdx (sf, i);
    song = dbGetByIdx (db, dbidx);
    ck_assert_ptr_nonnull (song);
  }

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

  songfilterSetNum (sf, SONG_FILTER_STATUS_PLAYABLE, SONG_FILTER_FOR_SELECTION);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_STATUS_PLAYABLE), 1);
  rv = songfilterProcess (sf, db);
  ck_assert_int_eq (rv, arv);

  songfilterSetNum (sf, SONG_FILTER_STATUS_PLAYABLE, SONG_FILTER_FOR_PLAYBACK);
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

  /* run the initial selection with the keyword filter active but no */
  /* keywords selected. this will reject any songs w/keywords. */
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_KEYWORD), 0);
  songfilterSetData (sf, SONG_FILTER_KEYWORD, NULL);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_KEYWORD), 1);

  arv = songfilterProcess (sf, db);

  lista = slistAlloc ("chk-sf-lista", LIST_ORDERED, NULL);
  slistSetNum (lista, "aaa", 0);
  listb = slistAlloc ("chk-sf-listb", LIST_ORDERED, NULL);
  slistSetNum (listb, "bbb", 0);
  listall = slistAlloc ("chk-sf-listall", LIST_ORDERED, NULL);
  slistSetNum (listall, "aaa", 0);
  slistSetNum (listall, "bbb", 0);

  songfilterSetData (sf, SONG_FILTER_KEYWORD, lista);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_KEYWORD), 1);
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
  slistFree (lista);
  slistFree (listb);
  slistFree (listall);
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

START_TEST(songfilter_favorite)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_favorite");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_FAVORITE), 0);

  songfilterSetNum (sf, SONG_FILTER_FAVORITE, 1);
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_FAVORITE), 1);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_search)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv, temprv [8];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_search");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_SEARCH), 0);

  /* find in tags, tag5 is upper case, tag8 does not exist */
  for (int i = 0; i < 8; ++i) {
    char  tmp [40];

    snprintf (tmp, sizeof (tmp), "tag%d", i + 1);
    songfilterSetData (sf, SONG_FILTER_SEARCH, tmp);
    rv = songfilterProcess (sf, db);
    ck_assert_int_lt (rv, arv);
    temprv [i] = rv;
  }
  ck_assert_int_eq (temprv [0], temprv [1]); // 1, 2, 3
  ck_assert_int_eq (temprv [1], temprv [2]); // 1, 2, 3
  ck_assert_int_eq (temprv [3], temprv [4]); // 4, 5
  ck_assert_int_eq (temprv [5], temprv [6]); // 6, 7
  ck_assert_int_eq (temprv [7], 0);  // "tag8"

  /* find in tags */
  songfilterSetData (sf, SONG_FILTER_SEARCH, "tag");
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, temprv [0] + temprv [1] + temprv [2] + temprv [3] +
      temprv [4] + temprv [5] + temprv [6] + temprv [7]);

  /* find in album-artist, artist */
  songfilterSetData (sf, SONG_FILTER_SEARCH, "artist");
  rv = songfilterProcess (sf, db);
  ck_assert_int_le (rv, arv);
  ck_assert_int_gt (rv, 0);

  /* failed search */
  songfilterSetData (sf, SONG_FILTER_SEARCH, "title");
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_eq (rv, 0);

  /* find in album-artist, album */
  songfilterSetData (sf, SONG_FILTER_SEARCH, "album");
  rv = songfilterProcess (sf, db);
  ck_assert_int_le (rv, arv);
  ck_assert_int_gt (rv, 0);

  /* find in title */
  songfilterSetData (sf, SONG_FILTER_SEARCH, "argentinetango");
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);

  /* find in notes */
  songfilterSetData (sf, SONG_FILTER_SEARCH, "notes1");
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);

  /* notes3 and notes4 are upper case */
  songfilterSetData (sf, SONG_FILTER_SEARCH, "notes3");
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);

  /* some upper/lower unicode in notes */
  songfilterSetData (sf, SONG_FILTER_SEARCH, "ÄÄÄÄääää");
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);

  /* find in keywords */
  songfilterSetData (sf, SONG_FILTER_SEARCH, "bbb");
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_SEARCH), 1);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_multi)
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

  /* waltz + search */
  songfilterSetNum (sf, SONG_FILTER_DANCE_IDX, wkey);
  songfilterSetData (sf, SONG_FILTER_SEARCH, "notes3");
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);

  /* waltz + search */
  songfilterSetNum (sf, SONG_FILTER_DANCE_IDX, wkey);
  songfilterSetData (sf, SONG_FILTER_SEARCH, "tag2");
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);

  /* waltz + rating */
  songfilterOff (sf, SONG_FILTER_SEARCH);
  songfilterSetNum (sf, SONG_FILTER_RATING, 3); // great
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);

  /* waltz + search + rating */
  songfilterOn (sf, SONG_FILTER_SEARCH);
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);

  songfilterFree (sf);
}
END_TEST

START_TEST(songfilter_bpm)
{
  songfilter_t  *sf;
  dbidx_t       arv, rv, rvw, rva, rvb;
  dance_t       *dances;
  ilist_t       *dflist;
  slist_t       *dlist;
  ilistidx_t    wkey;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_bpm");

  sf = songfilterAlloc ();
  arv = songfilterProcess (sf, db);

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dlist = danceGetDanceList (dances);
  wkey = slistGetNum (dlist, "Waltz");

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_DANCE_LIST), 0);

  dflist = ilistAlloc ("chk-sf-dnc-list", LIST_ORDERED);
  ilistSetNum (dflist, wkey, 0, 0);
  songfilterSetData (sf, SONG_FILTER_DANCE_LIST, dflist);
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);
  rvw = rv;

  songfilterDanceSet (sf, wkey, SONG_FILTER_BPM_LOW, 84);
  songfilterDanceSet (sf, wkey, SONG_FILTER_BPM_HIGH, 84);
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);
  ck_assert_int_lt (rv, rvw);
  rva = rv;

  songfilterDanceSet (sf, wkey, SONG_FILTER_BPM_LOW, 87);
  songfilterDanceSet (sf, wkey, SONG_FILTER_BPM_HIGH, 87);
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);
  ck_assert_int_lt (rv, rvw);
  rvb = rv;
  ck_assert_int_eq (rvw, rva + rvb);

  songfilterDanceSet (sf, wkey, SONG_FILTER_BPM_LOW, 84);
  songfilterDanceSet (sf, wkey, SONG_FILTER_BPM_HIGH, 87);
  rv = songfilterProcess (sf, db);
  ck_assert_int_lt (rv, arv);
  ck_assert_int_gt (rv, 0);
  ck_assert_int_eq (rv, rvw);

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_DANCE_LIST), 1);

  songfilterFree (sf);
}
END_TEST

/* check to make sure the various sorts are different from each other */
START_TEST(songfilter_sort)
{
  songfilter_t  *sf [sorttypesz];
  bool          d [sorttypesz];
  int           c, count;
  dbidx_t       dbidx, ldbidx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_sort");

  for (int i = 0; i < sorttypesz; ++i) {
    sf [i] = songfilterAlloc ();
    songfilterSetSort (sf [i], sorttypes [i]);
    songfilterProcess (sf [i], db);
    d [i] = false;
  }

  count = songfilterGetCount (sf [0]);
  for (int j = 0; j < count; ++j) {
    ldbidx = songfilterGetByIdx (sf [0], j);

    for (int i = 0; i < sorttypesz; ++i) {
      c = songfilterGetCount (sf [i]);
      ck_assert_int_eq (c, count);

      dbidx = songfilterGetByIdx (sf [i], j);
      if (dbidx != songfilterGetByIdx (sf [0], j)) {
        d [i] = true;
        d [0] = true;
      }
      if (dbidx != ldbidx) {
        if (i > 0) {
          d [i - 1] = true;
        }
        d [i] = true;
      }
      ldbidx = dbidx;
    }
  }
  count = 0;
  for (int i = 0; i < sorttypesz; ++i) {
    if (d [i]) {
      ++count;
    }
  }
  /* most; the check is very simplistic */
  ck_assert_int_ge (count, sorttypesz - 2);

  for (int i = 0; i < sorttypesz; ++i) {
    songfilterFree (sf [i]);
  }
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
  tcase_add_test (tc, songfilter_process);
  tcase_add_test (tc, songfilter_rating);
  tcase_add_test (tc, songfilter_clear);
  tcase_add_test (tc, songfilter_reset);
  tcase_add_test (tc, songfilter_off_on);
  tcase_add_test (tc, songfilter_get_byidx);
  tcase_add_test (tc, songfilter_genre);
  tcase_add_test (tc, songfilter_level);
  tcase_add_test (tc, songfilter_status);
  tcase_add_test (tc, songfilter_playable);
  tcase_add_test (tc, songfilter_keyword);
  tcase_add_test (tc, songfilter_dance_idx);
  tcase_add_test (tc, songfilter_dance_list);
  tcase_add_test (tc, songfilter_favorite);
  suite_add_tcase (s, tc);

  tc = tcase_create ("songfilter-search");
  tcase_set_tags (tc, "libbdj4 slow");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, songfilter_search);
  /* for some reason the mac is really slow */
  tcase_set_timeout (tc, 20.0);
  suite_add_tcase (s, tc);

  tc = tcase_create ("songfilter-b");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, songfilter_multi);
  tcase_add_test (tc, songfilter_bpm);
  suite_add_tcase (s, tc);

  tc = tcase_create ("songfilter-sort");
  tcase_set_tags (tc, "libbdj4 slow");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, songfilter_sort);
  /* for some reason the mac is really slow */
  tcase_set_timeout (tc, 20.0);
  suite_add_tcase (s, tc);

  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
