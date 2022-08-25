#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdjopt.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "dirop.h"
#include "filedata.h"
#include "filemanip.h"
#include "fileop.h"
#include "musicdb.h"
#include "slist.h"
#include "song.h"
#include "templateutil.h"

static char *dbfn = "tmp/musicdb.dat";

static char *songparsedata [] = {
    "FILE\n..argentinetango%d.mp3\n"
      "ADJUSTFLAGS\n..\n"
      "AFMODTIME\n..1660237221\n"
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
      "LASTUPDATED\n..1660237307\n"
      "RRN\n..%d\n",
    /* unicode filename */
    "FILE\n..IAmtheBest_내가제일잘나가%d.mp3\n"
      "ADJUSTFLAGS\n..\n"
      "AFMODTIME\n..1660237221\n"
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
      "LASTUPDATED\n..1660237307\n"
      "RRN\n..%d\n"
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
}

START_TEST(musicdb_open_new)
{
  musicdb_t *db;

  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);
  dbClose (db);
  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(musicdb_write)
{
  musicdb_t *db;
  char      tmp [200];
  size_t    len;
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;
  slist_t   *tlist;

  // fprintf (stdout, "  write\n");
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      len = strlen (songparsedata [i]);
      ndata = filedataReplace (songparsedata [i], &len, "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      free (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_FILE));
      fh = fileopOpen (tmp, "w");
      fclose (fh);

      tlist = songTagList (song);

      dbWrite (db, songGetStr (song, TAG_FILE),
          tlist, MUSICDB_ENTRY_NEW);
      songFree (song);
      ++count;
    }
  }

  dbClose (db);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(musicdb_overwrite)
{
  musicdb_t *db;
  char      tmp [200];
  size_t    len;
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;
  slist_t   *tlist;

  // fprintf (stdout, "  overwrite\n");
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      len = strlen (songparsedata [i]);
      ndata = filedataReplace (songparsedata [i], &len, "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      free (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_FILE));
      fh = fileopOpen (tmp, "w");
      fclose (fh);

      tlist = songTagList (song);

      dbWrite (db, songGetStr (song, TAG_FILE),
          tlist, songGetNum (song, TAG_RRN));
      songFree (song);
      ++count;
    }
  }

  dbClose (db);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

/* the test suite calls the cleanup before this test to remove the files */

START_TEST(musicdb_batch_write)
{
  musicdb_t *db;
  char      tmp [200];
  size_t    len;
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;
  slist_t   *tlist;

  // fprintf (stdout, "  batch write\n");
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);
  dbStartBatch (db);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      len = strlen (songparsedata [i]);
      ndata = filedataReplace (songparsedata [i], &len, "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      free (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_FILE));
      fh = fileopOpen (tmp, "w");
      fclose (fh);

      tlist = songTagList (song);

      dbWrite (db, songGetStr (song, TAG_FILE),
          tlist, MUSICDB_ENTRY_NEW);
      songFree (song);
      ++count;
    }
  }

  dbEndBatch (db);
  dbClose (db);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(musicdb_batch_overwrite)
{
  musicdb_t *db;
  char      tmp [200];
  size_t    len;
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;
  slist_t   *tlist;

  // fprintf (stdout, "  batch overwrite\n");
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);
  dbStartBatch (db);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      len = strlen (songparsedata [i]);
      ndata = filedataReplace (songparsedata [i], &len, "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      free (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_FILE));
      fh = fileopOpen (tmp, "w");
      fclose (fh);

      tlist = songTagList (song);

      dbWrite (db, songGetStr (song, TAG_FILE),
          tlist, songGetNum (song, TAG_RRN));
      songFree (song);
      ++count;
    }
  }

  dbEndBatch (db);
  dbClose (db);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

/* the test suite calls the cleanup before this test to remove the files */

START_TEST(musicdb_write_song)
{
  musicdb_t *db;
  char      tmp [200];
  size_t    len;
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;

  /* this method of adding a song to the database is not currently in use */
  /* 2022-8-18 */

  // fprintf (stdout, "  write song\n");
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      len = strlen (songparsedata [i]);
      ndata = filedataReplace (songparsedata [i], &len, "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, MUSICDB_ENTRY_NEW);
      free (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_FILE));
      fh = fileopOpen (tmp, "w");
      fclose (fh);

      dbWriteSong (db, song);
      songFree (song);
      ++count;
    }
  }

  dbClose (db);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(musicdb_overwrite_song)
{
  musicdb_t *db;
  char      tmp [200];
  size_t    len;
  song_t    *song;
  int       count;
  char      *ndata;
  FILE      *fh;

  // fprintf (stdout, "  overwrite song\n");
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      len = strlen (songparsedata [i]);
      ndata = filedataReplace (songparsedata [i], &len, "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      free (ndata);

      snprintf (tmp, sizeof (tmp), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
          songGetStr (song, TAG_FILE));
      fh = fileopOpen (tmp, "w");
      fclose (fh);

      dbWriteSong (db, song);
      songFree (song);
      ++count;
    }
  }

  dbClose (db);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(musicdb_load_get_byidx)
{
  musicdb_t *db;
  char      tmp [40];
  size_t    len;
  song_t    *song;
  song_t    *dbsong;
  int       count;
  char      *ndata;

  // fprintf (stdout, "   load byidx\n");
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);

  count = dbCount (db);
  ck_assert_int_eq (count, songparsedatasz * TEST_MAX);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      len = strlen (songparsedata [i]);
      ndata = filedataReplace (songparsedata [i], &len, "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      free (ndata);

      dbsong = dbGetByIdx (db, count);
      ck_assert_int_eq (songGetNum (dbsong, TAG_RRN), count + 1);
      ck_assert_str_eq (songGetStr (song, TAG_ARTIST),
          songGetStr (dbsong, TAG_ARTIST));
      songFree (song);
      ++count;
    }
  }

  dbClose (db);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(musicdb_load_get_byname)
{
  musicdb_t *db;
  char      tmp [40];
  size_t    len;
  song_t    *song;
  song_t    *dbsong;
  int       count;
  char      *ndata;

  // fprintf (stdout, "   load byname\n");
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);

  count = dbCount (db);
  ck_assert_int_eq (count, songparsedatasz * TEST_MAX);

  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      len = strlen (songparsedata [i]);
      ndata = filedataReplace (songparsedata [i], &len, "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      free (ndata);

      dbsong = dbGetByName (db, songGetStr (song, TAG_FILE));
      ck_assert_int_eq (songGetNum (dbsong, TAG_RRN), count + 1);
      ck_assert_str_eq (songGetStr (song, TAG_ARTIST),
          songGetStr (dbsong, TAG_ARTIST));
      songFree (song);
      ++count;
    }
  }

  dbClose (db);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST


START_TEST(musicdb_iterate)
{
  musicdb_t *db;
  char      tmp [40];
  size_t    len;
  song_t    *song;
  song_t    *dbsong;
  int       count;
  char      *ndata;
  dbidx_t   iteridx;
  dbidx_t   curridx;

  // fprintf (stdout, "   iterate\n");
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  bdjvarsdfloadInit ();

  db = dbOpen (dbfn);

  dbStartIterator (db, &iteridx);
  count = 0;
  for (int i = 0; i < songparsedatasz; ++i) {
    for (int j = 0; j < TEST_MAX; ++j) {
      song = songAlloc ();
      ck_assert_ptr_nonnull (song);
      snprintf (tmp, sizeof (tmp), "%02d", count);
      len = strlen (songparsedata [i]);
      ndata = filedataReplace (songparsedata [i], &len, "%d", tmp);
      songParse (song, ndata, count);
      songSetNum (song, TAG_RRN, count + 1);
      free (ndata);

      dbsong = dbIterate (db, &curridx, &iteridx);
      ck_assert_int_eq (songGetNum (dbsong, TAG_RRN), count + 1);
      ck_assert_str_eq (songGetStr (song, TAG_ARTIST),
          songGetStr (dbsong, TAG_ARTIST));
      songFree (song);
      ++count;
    }
  }

  dbClose (db);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(musicdb_load_entry)
{
  musicdb_t *db;
  song_t    *song;

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);

  song = dbGetByName (db, "argentinetango05.mp3");
  ck_assert_ptr_nonnull (song);
  ck_assert_str_eq (songGetStr (song, TAG_ARTIST), "artist05");

  songSetStr (song, TAG_ARTIST, "newartist");
  ck_assert_str_eq (songGetStr (song, TAG_ARTIST), "newartist");
  dbWriteSong (db, song);
  dbLoadEntry (db, songGetNum (song, TAG_DBIDX));

  song = dbGetByName (db, "argentinetango05.mp3");
  ck_assert_str_eq (songGetStr (song, TAG_ARTIST), "newartist");
  dbClose (db);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST


START_TEST(musicdb_db)
{
  musicdb_t *db;
  FILE      *fh;
  char      tbuff [200];
  int       count;
  int       racount;

  filemanipCopy ("test-templates/musicdb.dat", "data/musicdb.dat");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "test-music");
  bdjvarsdfloadInit ();
  db = dbOpen (dbfn);

  fh = fileopOpen ("data/musicdb.dat", "r");
  fgets (tbuff, sizeof (tbuff), fh); // version
  fgets (tbuff, sizeof (tbuff), fh); // comment
  fgets (tbuff, sizeof (tbuff), fh); // rasize
  fgets (tbuff, sizeof (tbuff), fh); // racount
  racount = atoi (tbuff);
  fclose (fh);

  count = dbCount (db);
  ck_assert_int_eq (count, racount);

  dbClose (db);

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(musicdb_cleanup)
{
  // fprintf (stdout, "   cleanup\n");
  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  fileopDelete (dbfn);
  diropDeleteDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  bdjoptCleanup ();
}

Suite *
musicdb_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("musicdb");
  tc = tcase_create ("musicdb-basic");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, musicdb_cleanup);
  tcase_add_test (tc, musicdb_open_new);
  suite_add_tcase (s, tc);

  tc = tcase_create ("musicdb-write");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, NULL);
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
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, musicdb_batch_write);
  tcase_add_test (tc, musicdb_load_get_byidx);
  tcase_add_test (tc, musicdb_load_get_byname);
  tcase_add_test (tc, musicdb_iterate);
  tcase_add_test (tc, musicdb_batch_overwrite);
  tcase_add_test (tc, musicdb_load_get_byidx);
  tcase_add_test (tc, musicdb_load_get_byname);
  tcase_add_test (tc, musicdb_iterate);
  tcase_add_test (tc, musicdb_cleanup);

  tc = tcase_create ("musicdb-write-song");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  suite_add_tcase (s, tc);
  tcase_add_test (tc, musicdb_write_song);
  tcase_add_test (tc, musicdb_load_get_byidx);
  tcase_add_test (tc, musicdb_load_get_byname);
  tcase_add_test (tc, musicdb_iterate);
  tcase_add_test (tc, musicdb_overwrite_song);
  tcase_add_test (tc, musicdb_load_get_byidx);
  tcase_add_test (tc, musicdb_load_get_byname);
  tcase_add_test (tc, musicdb_iterate);
  /* load entry needs a database */
  tcase_add_test (tc, musicdb_load_entry);
  tcase_add_test (tc, musicdb_cleanup);
  tcase_add_test (tc, musicdb_db);
  suite_add_tcase (s, tc);
  return s;
}
