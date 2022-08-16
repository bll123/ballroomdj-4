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

#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "datafile.h"
#include "ilist.h"
#include "nlist.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"
#include "templateutil.h"

START_TEST(song_alloc)
{
  song_t   *song = NULL;
  song_t   *songb = NULL;

  templateFileCopy ("dancetypes.txt", "dancetypes.txt");
  templateFileCopy ("dances.txt", "dances.txt");
  templateFileCopy ("genres.txt", "genres.txt");
  templateFileCopy ("levels.txt", "levels.txt");
  templateFileCopy ("ratings.txt", "ratings.txt");

  bdjvarsdfloadInit ();

  song = songAlloc ();
  ck_assert_ptr_nonnull (song);
  songFree (song);

  song = songAlloc ();
  songb = songAlloc ();
  ck_assert_ptr_nonnull (song);
  ck_assert_ptr_nonnull (songb);
  songFree (song);
  songFree (songb);

  bdjvarsdfloadCleanup ();
}
END_TEST

static char *songparsedata [] = {
    /* unix line endings */
    "FILE\n..01 Argentine Tango.mp3\n"
      "ADJUSTFLAGS\n..\n"
      "AFMODTIME\n..1660237221\n"
      "ALBUM\n..album\n"
      "ALBUMARTIST\n..albumartist\n"
      "ARTIST\n..artist\n"
      "BPM\n..200\n"
      "COMPOSER\n..composer\n"
      "CONDUCTOR\n..conductor\n"
      "DANCE\n..Waltz\n"
      "DANCELEVEL\n..Normal\n"
      "DANCERATING\n..Good\n"
      "DATE\n..2022-8-16\n"
      "DBADDDATE\n..2022-08-16\n"
      "DISC\n..1\n"
      "DISCTOTAL\n..3\n"
      "DURATION\n..304540\n"
      "FAVORITE\n..bluestar\n"
      "GENRE\n..Classical\n"
      "KEYWORD\n..keyword\n"
      "MQDISPLAY\n..Waltz\n"
      "NOTES\n..notes\n"
      "RECORDING_ID\n..\n"
      "SAMESONG\n..ss-0001\n"
      "SONGEND\n..\n"
      "SONGSTART\n..\n"
      "SPEEDADJUSTMENT\n..\n"
      "STATUS\n..New\n"
      "TAGS\n..tag1 tag2\n"
      "TITLE\n..title\n"
      "TRACK_ID\n..\n"
      "TRACKNUMBER\n..5\n"
      "TRACKTOTAL\n..10\n"
      "VOLUMEADJUSTPERC\n..4400\n"
      "WORK_ID\n..\n"
      "LASTUPDATED\n..1660237307\n"
      "RRN\n..1\n",
    /* windows line endings */
    "FILE\r\n..01 Argentine Tango.mp3\r\n"
      "ADJUSTFLAGS\r\n..\r\n"
      "AFMODTIME\r\n..1660237221\r\n"
      "ALBUM\r\n..album\r\n"
      "ALBUMARTIST\r\n..albumartist\r\n"
      "ARTIST\r\n..artist\r\n"
      "BPM\r\n..200\r\n"
      "COMPOSER\r\n..composer\r\n"
      "CONDUCTOR\r\n..conductor\r\n"
      "DANCE\r\n..Waltz\r\n"
      "DANCELEVEL\r\n..Normal\r\n"
      "DANCERATING\r\n..Good\r\n"
      "DATE\r\n..2022-8-16\r\n"
      "DBADDDATE\r\n..2022-08-16\r\n"
      "DISC\r\n..1\r\n"
      "DISCTOTAL\r\n..3\r\n"
      "DURATION\r\n..304540\r\n"
      "FAVORITE\r\n..bluestar\r\n"
      "GENRE\r\n..Classical\r\n"
      "KEYWORD\r\n..keyword\r\n"
      "MQDISPLAY\r\n..Waltz\r\n"
      "NOTES\r\n..notes\r\n"
      "RECORDING_ID\r\n..\r\n"
      "SAMESONG\r\n..ss-0001\r\n"
      "SONGEND\r\n..\r\n"
      "SONGSTART\r\n..\r\n"
      "SPEEDADJUSTMENT\r\n..\r\n"
      "STATUS\r\n..New\r\n"
      "TAGS\r\n..tag1 tag2\r\n"
      "TITLE\r\n..title\r\n"
      "TRACK_ID\r\n..\r\n"
      "TRACKNUMBER\r\n..5\r\n"
      "TRACKTOTAL\r\n..10\r\n"
      "VOLUMEADJUSTPERC\r\n..4400\r\n"
      "WORK_ID\r\n..\r\n"
      "LASTUPDATED\r\n..1660237307\r\n"
      "RRN\r\n..1\r\n"
};
enum {
  songparsedatasz = sizeof (songparsedata) / sizeof (char *),
};

START_TEST(song_parse)
{
  song_t    *song = NULL;
  char      *data;

  bdjvarsdfloadInit ();

  song = songAlloc ();
  ck_assert_ptr_nonnull (song);

  for (int i = 0; i < songparsedatasz; ++i) {
    data = strdup (songparsedata [i]);
    songParse (song, data, i);
    free (data);
  }
  songFree (song);

  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(song_parse_get)
{
  song_t      *song = NULL;
  char        *data;
  slist_t     *tlist;
  slistidx_t  iteridx;
  songfavoriteinfo_t *sfav;

  bdjvarsdfloadInit ();

  song = songAlloc ();
  ck_assert_ptr_nonnull (song);

  for (int i = 0; i < songparsedatasz; ++i) {
    data = strdup (songparsedata [i]);
    songParse (song, data, i);
    free (data);
    ck_assert_str_eq (songGetStr (song, TAG_ARTIST), "artist");
    ck_assert_str_eq (songGetStr (song, TAG_ALBUM), "album");
    ck_assert_int_eq (songGetNum (song, TAG_DISCNUMBER), 1);
    ck_assert_int_eq (songGetNum (song, TAG_TRACKNUMBER), 5);
    ck_assert_int_eq (songGetNum (song, TAG_TRACKTOTAL), 10);
    ck_assert_float_eq (songGetDouble (song, TAG_VOLUMEADJUSTPERC), 4.4);
    tlist = songGetList (song, TAG_TAGS);
    slistStartIterator (tlist, &iteridx);
    data = slistIterateKey (tlist, &iteridx);
    ck_assert_str_eq (data, "tag1");
    data = slistIterateKey (tlist, &iteridx);
    ck_assert_str_eq (data, "tag2");
    /* converted - these assume the standard data files */
    ck_assert_int_eq (songGetNum (song, TAG_GENRE), 2);
    ck_assert_int_eq (songGetNum (song, TAG_DANCE), 12);
    ck_assert_int_eq (songGetNum (song, TAG_DANCERATING), 2);
    ck_assert_int_eq (songGetNum (song, TAG_DANCELEVEL), 1);
    ck_assert_int_eq (songGetNum (song, TAG_STATUS), 0);
    ck_assert_int_eq (songGetNum (song, TAG_FAVORITE), SONG_FAVORITE_BLUE);
    sfav = songGetFavoriteData (song);
    ck_assert_int_eq (sfav->idx, SONG_FAVORITE_BLUE);
  }
  songFree (song);

  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(song_parse_set)
{
  song_t      *song = NULL;
  char        *data;
  songfavoriteinfo_t *sfav;

  bdjvarsdfloadInit ();

  song = songAlloc ();
  ck_assert_ptr_nonnull (song);

  for (int i = 0; i < songparsedatasz; ++i) {
    data = strdup (songparsedata [i]);
    songParse (song, data, i);
    free (data);

    songSetStr (song, TAG_ARTIST, "artistb");
    songSetStr (song, TAG_ALBUM, "albumb");
    songSetNum (song, TAG_DISCNUMBER, 2);
    songSetNum (song, TAG_TRACKNUMBER, 6);
    songSetNum (song, TAG_TRACKTOTAL, 11);
    songSetDouble (song, TAG_VOLUMEADJUSTPERC, 5.5);
    songChangeFavorite (song);

    ck_assert_str_eq (songGetStr (song, TAG_ARTIST), "artistb");
    ck_assert_str_eq (songGetStr (song, TAG_ALBUM), "albumb");
    ck_assert_int_eq (songGetNum (song, TAG_DISCNUMBER), 2);
    ck_assert_int_eq (songGetNum (song, TAG_TRACKNUMBER), 6);
    ck_assert_int_eq (songGetNum (song, TAG_TRACKTOTAL), 11);
    ck_assert_float_eq (songGetDouble (song, TAG_VOLUMEADJUSTPERC), 5.5);
    /* converted - these assume the standard data files */
    ck_assert_int_eq (songGetNum (song, TAG_FAVORITE), SONG_FAVORITE_PURPLE);
    sfav = songGetFavoriteData (song);
    ck_assert_int_eq (sfav->idx, SONG_FAVORITE_PURPLE);
  }
  songFree (song);

  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(song_audio_file)
{
}
END_TEST

START_TEST(song_display)
{
  song_t      *song = NULL;
  char        *data;
  int         rc;

  bdjvarsdfloadInit ();

  song = songAlloc ();
  ck_assert_ptr_nonnull (song);

  for (int i = 0; i < songparsedatasz; ++i) {
    data = strdup (songparsedata [i]);
    songParse (song, data, i);
    free (data);

    /* converted - these assume the standard data files */
    data = songDisplayString (song, TAG_GENRE);
    ck_assert_str_eq (data, "Classical");
    free (data);

    data = songDisplayString (song, TAG_DANCE);
    ck_assert_str_eq (data, "Waltz");
    free (data);

    data = songDisplayString (song, TAG_DANCERATING);
    ck_assert_str_eq (data, "Good");
    free (data);

    data = songDisplayString (song, TAG_DANCELEVEL);
    ck_assert_str_eq (data, "Normal");
    free (data);

    data = songDisplayString (song, TAG_STATUS);
    ck_assert_str_eq (data, "New");
    free (data);

    data = songDisplayString (song, TAG_FAVORITE);
    rc = strncmp (data, "<span", 5);
    ck_assert_int_eq (rc, 0);
    free (data);

    data = songDisplayString (song, TAG_TAGS);
    ck_assert_str_eq (songDisplayString (song, TAG_TAGS), "tag1 tag2");
    free (data);
  }
  songFree (song);

  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(song_tag_list)
{
}
END_TEST

START_TEST(song_dur_cache)
{
  song_t      *song = NULL;

  bdjvarsdfloadInit ();

  ck_assert_int_eq (songGetDurCache (song), -1);
  song = songAlloc ();
  ck_assert_ptr_nonnull (song);
  ck_assert_int_eq (songGetDurCache (song), -1);
  songSetDurCache (song, 20000);
  ck_assert_int_eq (songGetDurCache (song), 20000);
  songFree (song);

  bdjvarsdfloadCleanup ();
}
END_TEST

Suite *
song_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("song Suite");
  tc = tcase_create ("song");
  tcase_add_test (tc, song_alloc);
  tcase_add_test (tc, song_parse);
  tcase_add_test (tc, song_parse_get);
  tcase_add_test (tc, song_parse_set);
  tcase_add_test (tc, song_audio_file);
  tcase_add_test (tc, song_display);
  tcase_add_test (tc, song_tag_list);
  tcase_add_test (tc, song_dur_cache);
  suite_add_tcase (s, tc);
  return s;
}
