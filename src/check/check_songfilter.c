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
#include "filemanip.h"
#include "log.h"
#include "songfilter.h"
#include "check_bdj.h"
#include "musicdb.h"
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
  "UPDATETIME",
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
  dbidx_t       rv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- songfilter_rating");

  sf = songfilterAlloc ();

  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 0);
  songfilterSetNum (sf, SONG_FILTER_RATING, 3);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 1);
  songfilterOff (sf, SONG_FILTER_RATING);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 0);
  songfilterOn (sf, SONG_FILTER_RATING);
  ck_assert_int_eq (songfilterInUse (sf, SONG_FILTER_RATING), 1);

  rv = songfilterProcess (sf, db);

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
  suite_add_tcase (s, tc);
  return s;
}

