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

#include "audiosrc.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjregex.h"
#include "bdjvars.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "musicdb.h"
#include "dirop.h"
#include "filemanip.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"
#include "templateutil.h"

static char *dbfn = "tmp/musicdb.dat";

static char *songparsedata [] = {
    "URI\n..argentinetango%d.mp3\n"
      "ADJUSTFLAGS\n..\n"
      "ALBUM\n..album%d\n"
      "ALBUMARTIST\n..albumartist%d\n"
      "ARTIST\n..artist%d\n"
      "BPM\n..200\n"
      "COMPOSER\n..composer%d\n"
      "CONDUCTOR\n..conductor%d\n"
      "DANCE\n..Waltz\n"
      "DANCELEVEL\n..Normal\n"
      "DANCERATING\n..Good\n"
      "DATE\n..2022-8-18\n"
      "DBADDDATE\n..2022-08-18\n"
      "DISC\n..1\n"
      "DISCTOTAL\n..3\n"
      "DURATION\n..304540\n"
      "FAVORITE\n..bluestar\n"
      "GENRE\n..Classical\n"
      "KEYWORD\n..keyword%d\n"
      "MQDISPLAY\n..Waltz\n"
      "NOTES\n..notes\n"
      "RECORDING_ID\n..\n"
      "SAMESONG\n..%d\n"
      "SONGEND\n..\n"
      "SONGSTART\n..\n"
      "SPEEDADJUSTMENT\n..\n"
      "STATUS\n..New\n"
      "TAGS\n..tag1 tag2\n"
      "TITLE\n..title%d\n"
      "TRACK_ID\n..trackid%d\n"
      "TRACKNUMBER\n..5\n"
      "TRACKTOTAL\n..10\n"
      "VOLUMEADJUSTPERC\n..4400\n"
      "WORK_ID\n..workid%d\n"
      "LASTUPDATED\n..1660237307\n",
    /* unicode filename */
    "URI\n..IAmtheBest_내가제일잘나가%d.mp3\n"
      "ADJUSTFLAGS\n..\n"
      "ALBUM\n..album%d\n"
      "ALBUMARTIST\n..albumartist%d\n"
      "ARTIST\n..artist%d\n"
      "BPM\n..200\n"
      "COMPOSER\n..composer%d\n"
      "CONDUCTOR\n..conductor%d\n"
      "DANCE\n..Waltz\n"
      "DANCELEVEL\n..Normal\n"
      "DANCERATING\n..Good\n"
      "DATE\n..2022-8-18\n"
      "DBADDDATE\n..2022-08-18\n"
      "DISC\n..1\n"
      "DISCTOTAL\n..3\n"
      "DURATION\n..304540\n"
      "FAVORITE\n..bluestar\n"
      "GENRE\n..Classical\n"
      "KEYWORD\n..keyword%d\n"
      "MQDISPLAY\n..\n"
      "NOTES\n..notes%d\n"
      "RECORDING_ID\n..recording%d\n"
      "SAMESONG\n..%d\n"
      "SONGEND\n..\n"
      "SONGSTART\n..\n"
      "SPEEDADJUSTMENT\n..\n"
      "STATUS\n..New\n"
      "TAGS\n..tag1 tag2\n"
      "TITLE\n..title%d\n"
      "TRACK_ID\n..\n"
      "TRACKNUMBER\n..5\n"
      "TRACKTOTAL\n..10\n"
      "VOLUMEADJUSTPERC\n..4400\n"
      "WORK_ID\n..workid%d\n"
      "LASTUPDATED\n..1660237307\n",
};
enum {
  songparsedatasz = sizeof (songparsedata) / sizeof (char *),
  TEST_MAX = 20,
};

static void
setup (void)
{
  templateFileCopy ("dancetypes.txt", "dancetypes.txt");
  templateFileCopy ("dances.txt", "dances.txt");
  templateFileCopy ("genres.txt", "genres.txt");
  templateFileCopy ("levels.txt", "levels.txt");
  templateFileCopy ("ratings.txt", "ratings.txt");
  filemanipCopy ("test-templates/status.txt", "data/status.txt");
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  bdjvarsInit ();
  bdjvarsdfloadInit ();
  audiosrcInit ();
}

static void
teardown (void)
{
  audiosrcCleanup ();
  bdjvarsdfloadCleanup ();
  bdjvarsCleanup ();
  bdjoptCleanup ();
}

START_TEST(musicdb_open_new)
{
  musicdb_t *db;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_open_new");
  mdebugSubTag ("musicdb_open_new");

  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);
  dbClose (db);
  /* on unix, the database does not get created */
  /* on windows, the database does get created */
}
END_TEST

START_TEST(musicdb_write)
{
  musicdb_t *db;
  char      tmp [200];
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;
  int       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_write");
  mdebugSubTag ("musicdb_write");

  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      ndata = regexReplaceLiteral (songparsedata [i], "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      mdfree (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_URI));
      fh = fileopOpen (tmp, "w");
      mdextfclose (fh);
      fclose (fh);

      dbWriteSong (db, song);
      songFree (song);
      ++count;
    }
  }

  rc = fileopFileExists (dbfn);
  ck_assert_int_eq (rc, 1);

  dbClose (db);
}
END_TEST

START_TEST(musicdb_overwrite)
{
  musicdb_t *db;
  char      tmp [200];
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;
  int       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_overwrite");
  mdebugSubTag ("musicdb_overwrite");

  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      ndata = regexReplaceLiteral (songparsedata [i], "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      mdfree (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_URI));
      fh = fileopOpen (tmp, "w");
      mdextfclose (fh);
      fclose (fh);

      dbWriteSong (db, song);
      songFree (song);
      ++count;
    }
  }

  rc = fileopFileExists (dbfn);
  ck_assert_int_eq (rc, 1);

  dbClose (db);
}
END_TEST

/* the test suite calls the cleanup before this test to remove the files */

START_TEST(musicdb_batch_write)
{
  musicdb_t *db;
  char      tmp [200];
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;
  int       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_batch_write");
  mdebugSubTag ("musicdb_batch_write");

  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);
  dbStartBatch (db);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      ndata = regexReplaceLiteral (songparsedata [i], "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      mdfree (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_URI));
      fh = fileopOpen (tmp, "w");
      mdextfclose (fh);
      fclose (fh);

      dbWriteSong (db, song);
      songFree (song);
      ++count;
    }
  }

  rc = fileopFileExists (dbfn);
  ck_assert_int_eq (rc, 1);

  dbEndBatch (db);
  dbClose (db);
}
END_TEST

START_TEST(musicdb_batch_overwrite)
{
  musicdb_t *db;
  char      tmp [200];
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_batch_overwrite");
  mdebugSubTag ("musicdb_batch_overwrite");

  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);
  dbStartBatch (db);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      ndata = regexReplaceLiteral (songparsedata [i], "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      mdfree (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_URI));
      fh = fileopOpen (tmp, "w");
      mdextfclose (fh);
      fclose (fh);

      dbWriteSong (db, song);
      songFree (song);
      ++count;
    }
  }

  dbEndBatch (db);
  dbClose (db);
}
END_TEST

/* the test suite calls the cleanup before this test to remove the files */

START_TEST(musicdb_write_song)
{
  musicdb_t *db;
  char      tmp [200];
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;
  int       rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_write_song");
  mdebugSubTag ("musicdb_write_song");

  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      ndata = regexReplaceLiteral (songparsedata [i], "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, MUSICDB_ENTRY_NEW);
      mdfree (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_URI));
      fh = fileopOpen (tmp, "w");
      mdextfclose (fh);
      fclose (fh);

      dbWriteSong (db, song);
      songFree (song);
      ++count;
    }
  }

  rc = fileopFileExists (dbfn);
  ck_assert_int_eq (rc, 1);

  dbClose (db);
}
END_TEST

START_TEST(musicdb_overwrite_song)
{
  musicdb_t *db;
  char      tmp [200];
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_overwrite_song");
  mdebugSubTag ("musicdb_overwrite_song");

  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      ndata = regexReplaceLiteral (songparsedata [i], "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      mdfree (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_URI));
      fh = fileopOpen (tmp, "w");
      mdextfclose (fh);
      fclose (fh);

      dbWriteSong (db, song);
      songFree (song);
      ++count;
    }
  }

  dbClose (db);

}
END_TEST

START_TEST(musicdb_load_get_byidx)
{
  musicdb_t *db;
  char      tmp [40];
  song_t    *song;
  song_t    *dbsong;
  int       count;
  char      *ndata;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_load_get_byidx");
  mdebugSubTag ("musicdb_load_get_byidx");

  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  count = dbCount (db);
  ck_assert_int_eq (count, songparsedatasz * TEST_MAX);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      ndata = regexReplaceLiteral (songparsedata [i], "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      mdfree (ndata);

      dbsong = dbGetByIdx (db, count);
      ck_assert_ptr_nonnull (dbsong);
      ck_assert_int_eq (songGetNum (dbsong, TAG_RRN), count + 1);
      ck_assert_int_eq (songGetNum (dbsong, TAG_DBIDX), count);
      ck_assert_str_eq (songGetStr (song, TAG_ARTIST),
          songGetStr (dbsong, TAG_ARTIST));
      songFree (song);
      ++count;
    }
  }

  dbClose (db);

}
END_TEST

START_TEST(musicdb_load_get_byname)
{
  musicdb_t *db;
  char      tmp [40];
  song_t    *song;
  song_t    *dbsong;
  int       count;
  char      *ndata;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_load_get_byname");
  mdebugSubTag ("musicdb_load_get_byname");

  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  count = dbCount (db);
  ck_assert_int_eq (count, songparsedatasz * TEST_MAX);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      ndata = regexReplaceLiteral (songparsedata [i], "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      mdfree (ndata);

      dbsong = dbGetByName (db, songGetStr (song, TAG_URI));
      ck_assert_ptr_nonnull (dbsong);
      ck_assert_int_eq (songGetNum (dbsong, TAG_RRN), count + 1);
      ck_assert_int_eq (songGetNum (dbsong, TAG_DBIDX), count);
      ck_assert_str_eq (songGetStr (song, TAG_ARTIST),
          songGetStr (dbsong, TAG_ARTIST));

      songFree (song);
      ++count;
    }
  }

  dbClose (db);

}
END_TEST


START_TEST(musicdb_iterate)
{
  musicdb_t *db;
  char      tmp [40];
  song_t    *song;
  song_t    *dbsong;
  int       count;
  char      *ndata;
  dbidx_t   iteridx;
  dbidx_t   curridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_iterate");
  mdebugSubTag ("musicdb_iterate");

  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  dbStartIterator (db, &iteridx);
  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      ndata = regexReplaceLiteral (songparsedata [i], "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      mdfree (ndata);

      dbsong = dbIterate (db, &curridx, &iteridx);
      ck_assert_int_eq (songGetNum (dbsong, TAG_RRN), count + 1);
      ck_assert_int_eq (songGetNum (dbsong, TAG_DBIDX), count);
      ck_assert_str_eq (songGetStr (song, TAG_ARTIST),
          songGetStr (dbsong, TAG_ARTIST));
      songFree (song);
      ++count;
    }
  }

  dbClose (db);

}
END_TEST

START_TEST(musicdb_load_entry)
{
  musicdb_t *db;
  song_t    *song;
  song_t    *dbsong;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_load_entry");
  mdebugSubTag ("musicdb_load_entry");

  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  song = dbGetByName (db, "argentinetango05.mp3");
  ck_assert_ptr_nonnull (song);
  ck_assert_str_eq (songGetStr (song, TAG_ARTIST), "artist05");

  songSetStr (song, TAG_ARTIST, "newartist");
  ck_assert_str_eq (songGetStr (song, TAG_ARTIST), "newartist");
  dbWriteSong (db, song);
  dbLoadEntry (db, songGetNum (song, TAG_DBIDX));
  /* and now song is no longer valid */

  dbsong = dbGetByName (db, "argentinetango05.mp3");
  ck_assert_ptr_nonnull (dbsong);
  ck_assert_str_eq (songGetStr (dbsong, TAG_ARTIST), "newartist");
  dbClose (db);

}
END_TEST


START_TEST(musicdb_temp)
{
  musicdb_t *db;
  song_t    *song;
  song_t    *dbsong;
  dbidx_t   dbidx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_temp");
  mdebugSubTag ("musicdb_temp");

  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  song = songAlloc ();
  ck_assert_ptr_nonnull (song);
  songSetStr (song, TAG_URI, "tmp/waltz.mp3");
  songSetNum (song, TAG_DURATION, 20000);
  songSetStr (song, TAG_ARTIST, "temp-artist");
  songSetStr (song, TAG_TITLE, "temp-title");
  songSetNum (song, TAG_DB_FLAGS, MUSICDB_STD);
  /* sets the temporary flag */
  dbidx = dbAddTemporarySong (db, song);
  ck_assert_int_ne (dbidx, -1);
  ck_assert_int_eq (MUSICDB_TEMP, songGetNum (song, TAG_DB_FLAGS));

  dbsong = dbGetByIdx (db, dbidx);
  ck_assert_ptr_nonnull (dbsong);
  ck_assert_int_eq (dbidx, songGetNum (dbsong, TAG_DBIDX));
  ck_assert_int_eq (MUSICDB_TEMP, songGetNum (dbsong, TAG_DB_FLAGS));
  ck_assert_ptr_eq (dbsong, song);

  ++dbidx;
  dbsong = dbGetByIdx (db, dbidx);
  ck_assert_ptr_null (dbsong);

  dbClose (db);

}
END_TEST

START_TEST(musicdb_markremove)
{
  musicdb_t *db;
  song_t    *song;
  dbidx_t   dbidx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_markremove");
  mdebugSubTag ("musicdb_markremove");

  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  song = dbGetByName (db, "argentinetango05.mp3");
  dbidx = songGetNum (song, TAG_DBIDX);
  dbMarkEntryRemoved (db, dbidx);
  ck_assert_int_eq (MUSICDB_REMOVE_MARK, songGetNum (song, TAG_DB_FLAGS));
  dbClearEntryRemoved (db, dbidx);
  ck_assert_int_eq (MUSICDB_STD, songGetNum (song, TAG_DB_FLAGS));
  dbMarkEntryRemoved (db, dbidx);

  dbClose (db);
}
END_TEST

START_TEST(musicdb_rename)
{
  musicdb_t *db;
  song_t    *song;
  song_t    *tsong;
  song_t    *dbsong;
  dbidx_t   dbidx;
  dbidx_t   tdbidx;
  dbidx_t   curridx;
  dbidx_t   iteridx;
  char      ouri [MAXPATHLEN];
  char      nuri [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_rename");
  mdebugSubTag ("musicdb_rename");

  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);
  bdjoptSetNum (OPT_G_AUTOORGANIZE, true);

  song = dbGetByName (db, "argentinetango05.mp3");
  ck_assert_ptr_nonnull (song);
  dbidx = songGetNum (song, TAG_DBIDX);
  snprintf (ouri, sizeof (ouri), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
      songGetStr (song, TAG_URI));

  songSetStr (song, TAG_URI, "at-rename.mp3");
  snprintf (nuri, sizeof (nuri), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
      songGetStr (song, TAG_URI));
  filemanipMove (ouri, nuri);
  dbWriteSong (db, song);

  dbLoadEntry (db, dbidx);
  dbMarkEntryRenamed (db, "argentinetango05.mp3", "at-rename.mp3", dbidx);

  dbsong = dbGetByName (db, "argentinetango05.mp3");
  ck_assert_ptr_null (dbsong);

  dbsong = dbGetByIdx (db, dbidx);
  ck_assert_ptr_nonnull (dbsong);
  ck_assert_str_eq ("at-rename.mp3", songGetStr (dbsong, TAG_URI));

  dbStartIterator (db, &iteridx);
  while ((tsong = dbIterate (db, &curridx, &iteridx)) != NULL) {
    tdbidx = songGetNum (tsong, TAG_DBIDX);
    if (tdbidx == dbidx) {
      ck_assert_str_eq ("at-rename.mp3", songGetStr (tsong, TAG_URI));
      break;
    }
  }

  dbClose (db);
}
END_TEST

/* removesong is normally only used with remote songs */
START_TEST(musicdb_removesong)
{
  musicdb_t *db;
  song_t    *song;
  song_t    *dbsong;
  dbidx_t   dbidx;
  dbidx_t   curridx;
  dbidx_t   iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_removesong");
  mdebugSubTag ("musicdb_removesong");

  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  dbsong = dbGetByName (db, "argentinetango11.mp3");
  ck_assert_ptr_nonnull (dbsong);
  dbidx = songGetNum (dbsong, TAG_DBIDX);

  dbRemoveSong (db, dbidx);

  dbClose (db);
  db = dbOpen (dbfn);

  ck_assert_ptr_nonnull (db);
  dbsong = dbGetByName (db, "argentinetango11.mp3");
  ck_assert_ptr_null (dbsong);

  dbStartIterator (db, &iteridx);
  while ((song = dbIterate (db, &curridx, &iteridx)) != NULL) {
    ck_assert_str_ne ("argentinetango11.mp3", songGetStr (song, TAG_URI));
  }

  dbClose (db);
}
END_TEST

START_TEST(musicdb_db)
{
  musicdb_t *db;
  FILE      *fh;
  char      tbuff [200];
  int       count;
  int       racount;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_db");
  mdebugSubTag ("musicdb_db");

  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");

  db = dbOpen (dbfn);
  ck_assert_ptr_nonnull (db);

  fh = fileopOpen ("data/musicdb.dat", "r");
  (void) ! fgets (tbuff, sizeof (tbuff), fh); // version
  (void) ! fgets (tbuff, sizeof (tbuff), fh); // comment
  (void) ! fgets (tbuff, sizeof (tbuff), fh); // rasize
  (void) ! fgets (tbuff, sizeof (tbuff), fh); // racount
  racount = atoi (tbuff);
  mdextfclose (fh);
  fclose (fh);

  count = dbCount (db);
  ck_assert_int_eq (count, racount);

  dbClose (db);
}
END_TEST

START_TEST(musicdb_cleanup)
{
  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- musicdb_cleanup");
  mdebugSubTag ("musicdb_cleanup");

  fileopDelete (dbfn);
  diropDeleteDir (bdjoptGetStr (OPT_M_DIR_MUSIC), DIROP_ALL);
}
END_TEST

Suite *
musicdb_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("musicdb");

  tc = tcase_create ("musicdb-basic");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, musicdb_cleanup);
  tcase_add_test (tc, musicdb_open_new);
  suite_add_tcase (s, tc);

  tc = tcase_create ("musicdb-write");
  tcase_set_tags (tc, "libbdj4");
  tcase_set_timeout (tc, 4.0);
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, musicdb_write);
  tcase_add_test (tc, musicdb_load_get_byidx);
  tcase_add_test (tc, musicdb_load_get_byname);
  tcase_add_test (tc, musicdb_iterate);
  tcase_add_test (tc, musicdb_overwrite);
  tcase_add_test (tc, musicdb_load_get_byidx);
  tcase_add_test (tc, musicdb_load_get_byname);
  tcase_add_test (tc, musicdb_iterate);
  tcase_add_test (tc, musicdb_cleanup);
  suite_add_tcase (s, tc);

  tc = tcase_create ("musicdb-batch-write");
  tcase_set_tags (tc, "libbdj4");
  tcase_set_timeout (tc, 4.0);
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, musicdb_batch_write);
  tcase_add_test (tc, musicdb_load_get_byidx);
  tcase_add_test (tc, musicdb_load_get_byname);
  tcase_add_test (tc, musicdb_iterate);
  tcase_add_test (tc, musicdb_batch_overwrite);
  tcase_add_test (tc, musicdb_load_get_byidx);
  tcase_add_test (tc, musicdb_load_get_byname);
  tcase_add_test (tc, musicdb_iterate);
  tcase_add_test (tc, musicdb_cleanup);
  suite_add_tcase (s, tc);

  tc = tcase_create ("musicdb-write-song");
  tcase_set_tags (tc, "libbdj4");
  tcase_set_timeout (tc, 4.0);
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, musicdb_write_song);
  tcase_add_test (tc, musicdb_load_get_byidx);
  tcase_add_test (tc, musicdb_load_get_byname);
  tcase_add_test (tc, musicdb_iterate);
  tcase_add_test (tc, musicdb_overwrite_song);
  tcase_add_test (tc, musicdb_load_get_byidx);
  tcase_add_test (tc, musicdb_load_get_byname);
  tcase_add_test (tc, musicdb_iterate);
  tcase_add_test (tc, musicdb_load_entry);
  tcase_add_test (tc, musicdb_temp);
  tcase_add_test (tc, musicdb_markremove);
  tcase_add_test (tc, musicdb_rename);
  tcase_add_test (tc, musicdb_removesong);
  tcase_add_test (tc, musicdb_cleanup);
  tcase_add_test (tc, musicdb_db);
  suite_add_tcase (s, tc);

  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
