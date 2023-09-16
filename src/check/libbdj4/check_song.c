/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

#include "bdjopt.h"
#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "datafile.h"
#include "dirop.h"
#include "filemanip.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "song.h"
#include "songfav.h"
#include "tagdef.h"
#include "templateutil.h"

static void
setup (void)
{
  templateFileCopy ("dancetypes.txt", "dancetypes.txt");
  templateFileCopy ("dances.txt", "dances.txt");
  templateFileCopy ("genres.txt", "genres.txt");
  templateFileCopy ("levels.txt", "levels.txt");
  templateFileCopy ("ratings.txt", "ratings.txt");
  templateFileCopy ("favorites.txt", "favorites.txt");
  filemanipCopy ("test-templates/status.txt", "data/status.txt");
}

START_TEST(song_alloc)
{
  song_t   *song = NULL;
  song_t   *songb = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- song_alloc");
  mdebugSubTag ("song_alloc");

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
    "FILE\n..argentinetango.mp3\n"
      "ADJUSTFLAGS\n..NTS\n"
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
      "SAMESONG\n..3\n"
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
      "LASTUPDATED\n..1660237307\n",
    /* windows line endings */
    "FILE\r\n..waltz.mp3\r\n"
      "ADJUSTFLAGS\r\n..NT\r\n"
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
      "SAMESONG\r\n..4\r\n"
      "SONGEND\r\n..\r\n"
      "SONGSTART\r\n..\r\n"
      "SPEEDADJUSTMENT\r\n..\r\n"
      "STATUS\r\n..New\r\n"
      "TAGS\r\n..tag1 tag2\r\n"
      "TITLE\r\n..title\r\n"
      "TRACK_ID\r\n..\r\n"
      "TRACKNUMBER\r\n..5\r\n"
      "TRACKTOTAL\r\n..10\r\n"
      "VOLUMEADJUSTPERC\r\n..-4400\r\n"
      "WORK_ID\r\n..\r\n"
      "LASTUPDATED\r\n..1660237307\r\n",
    /* unix line endings, unicode filename */
    "FILE\n..IAmtheBest_내가제일잘나가.mp3\n"
      "ADJUSTFLAGS\n..\n"
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
      "SAMESONG\n..5\n"
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
      "LASTUPDATED\n..1660237307\n",
    /* unix line endings, not all data*/
    "FILE\n..chacha.mp3\n"
      "ALBUM\n..album\n"
      "ALBUMARTIST\n..albumartist\n"
      "ARTIST\n..artist\n"
      "DANCE\n..Cha Cha\n"
      "DANCELEVEL\n..Normal\n"
      "DANCERATING\n..Good\n"
      "DURATION\n..304540\n",
};
enum {
  songparsedatasz = sizeof (songparsedata) / sizeof (char *),
};

START_TEST(song_parse)
{
  song_t    *song = NULL;
  char      *data;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- song_parse");
  mdebugSubTag ("song_parse");

  bdjvarsdfloadInit ();

  for (int i = 0; i < songparsedatasz; ++i) {
    song = songAlloc ();
    data = mdstrdup (songparsedata [i]);
    songParse (song, data, i);
    mdfree (data);
    ck_assert_int_eq (songIsChanged (song), 0);
    ck_assert_int_eq (songHasSonglistChange (song), 0);
    songFree (song);
  }

  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(song_parse_get)
{
  song_t      *song = NULL;
  char        *data;
  const char  *cdata;
  slist_t     *tlist;
  slistidx_t  iteridx;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- song_parse_get");
  mdebugSubTag ("song_parse_get");

  bdjvarsdfloadInit ();

  for (int i = 0; i < songparsedatasz; ++i) {
    double          dval;
    int             rc;
    datafileconv_t  conv;

    song = songAlloc ();
    data = mdstrdup (songparsedata [i]);
    songParse (song, data, i);
    mdfree (data);
    ck_assert_int_eq (songIsChanged (song), 0);
    ck_assert_int_eq (songHasSonglistChange (song), 0);
    ck_assert_str_eq (songGetStr (song, TAG_ARTIST), "artist");
    ck_assert_str_eq (songGetStr (song, TAG_ALBUMARTIST), "albumartist");
    ck_assert_str_eq (songGetStr (song, TAG_ALBUM), "album");
    ck_assert_int_eq (songGetNum (song, TAG_DANCERATING), 2);
    ck_assert_int_eq (songGetNum (song, TAG_DANCELEVEL), 1);
    if (i < 3) {
      if (i == 0) {
        ck_assert_int_eq (songGetNum (song, TAG_ADJUSTFLAGS), 7);
      }
      if (i == 1) {
        ck_assert_int_eq (songGetNum (song, TAG_ADJUSTFLAGS), 3);
      }
      if (i == 2) {
        ck_assert_int_eq (songGetNum (song, TAG_ADJUSTFLAGS), 0);
      }
      ck_assert_str_eq (songGetStr (song, TAG_COMPOSER), "composer");
      ck_assert_int_eq (songGetNum (song, TAG_DISCNUMBER), 1);
      ck_assert_int_eq (songGetNum (song, TAG_TRACKNUMBER), 5);
      ck_assert_int_eq (songGetNum (song, TAG_TRACKTOTAL), 10);
      dval = songGetDouble (song, TAG_VOLUMEADJUSTPERC);
      rc = dval == 4.4 || dval == -4.4;
      ck_assert_int_eq (rc, 1);
      tlist = songGetList (song, TAG_TAGS);
      slistStartIterator (tlist, &iteridx);
      cdata = slistIterateKey (tlist, &iteridx);
      ck_assert_str_eq (cdata, "tag1");
      cdata = slistIterateKey (tlist, &iteridx);
      ck_assert_str_eq (cdata, "tag2");
      /* converted - these assume the standard data files */
      ck_assert_int_eq (songGetNum (song, TAG_GENRE), 2);
      ck_assert_int_eq (songGetNum (song, TAG_DANCE), 11);
      ck_assert_int_eq (songGetNum (song, TAG_STATUS), 0);

      conv.invt = VALUE_STR;
      conv.str = "bluestar";
      songFavoriteConv (&conv);
      ck_assert_int_eq (songGetNum (song, TAG_FAVORITE), conv.num);
    }
    if (i == 3) {
      ck_assert_ptr_null (songGetStr (song, TAG_COMPOSER));
    }
    songFree (song);
  }

  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(song_parse_set)
{
  song_t      *song = NULL;
  char        *data;
  const char  *cdata;
  slist_t     *tlist;
  slistidx_t  iteridx;
  datafileconv_t conv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- song_parse_set");
  mdebugSubTag ("song_parse_set");

  bdjvarsdfloadInit ();

  for (int i = 0; i < songparsedatasz; ++i) {
    song = songAlloc ();
    data = mdstrdup (songparsedata [i]);
    songParse (song, data, i);
    mdfree (data);

    ck_assert_int_eq (songIsChanged (song), 0);
    ck_assert_int_eq (songHasSonglistChange (song), 0);
    songSetStr (song, TAG_ARTIST, "artistb");
    songSetStr (song, TAG_ALBUM, "albumb");

    songSetNum (song, TAG_DISCNUMBER, 2);
    songSetNum (song, TAG_TRACKNUMBER, 6);
    songSetNum (song, TAG_TRACKTOTAL, 11);
    songSetDouble (song, TAG_VOLUMEADJUSTPERC, -5.5);
    data = mdstrdup ("tag5 tag4");
    songSetList (song, TAG_TAGS, data);
    mdfree (data);
    songChangeFavorite (song);

    ck_assert_str_eq (songGetStr (song, TAG_ARTIST), "artistb");
    ck_assert_str_eq (songGetStr (song, TAG_ALBUM), "albumb");
    ck_assert_int_eq (songIsChanged (song), 1);
    ck_assert_int_eq (songHasSonglistChange (song), 0);
    songSetStr (song, TAG_TITLE, "title");
    ck_assert_int_eq (songHasSonglistChange (song), 1);
    songClearChanged (song);
    ck_assert_int_eq (songIsChanged (song), 0);

    ck_assert_int_eq (songGetNum (song, TAG_DISCNUMBER), 2);
    ck_assert_int_eq (songGetNum (song, TAG_TRACKNUMBER), 6);
    ck_assert_int_eq (songGetNum (song, TAG_TRACKTOTAL), 11);

    if (i < 3) {
      if (i == 0) {
        ck_assert_int_eq (songGetNum (song, TAG_ADJUSTFLAGS), 7);
      }
      if (i == 1) {
        ck_assert_int_eq (songGetNum (song, TAG_ADJUSTFLAGS), 3);
      }
      if (i == 2) {
        ck_assert_int_eq (songGetNum (song, TAG_ADJUSTFLAGS), 0);
      }
    }
    ck_assert_float_eq (songGetDouble (song, TAG_VOLUMEADJUSTPERC), -5.5);
    tlist = songGetList (song, TAG_TAGS);
    slistStartIterator (tlist, &iteridx);
    cdata = slistIterateKey (tlist, &iteridx);
    ck_assert_str_eq (cdata, "tag5");
    cdata = slistIterateKey (tlist, &iteridx);
    ck_assert_str_eq (cdata, "tag4");

    conv.invt = VALUE_STR;
    if (i < 3) {
      conv.str = "purplestar";
    } else {
      conv.str = "redstar";
    }
    songFavoriteConv (&conv);
    ck_assert_int_eq (songGetNum (song, TAG_FAVORITE), conv.num);
    songFree (song);
  }

  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(song_audio_file)
{
  song_t      *song = NULL;
  char        *data;
  FILE        *fh;
  char        tbuff [200];
  bool        rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- song_audio_file");
  mdebugSubTag ("song_audio_file");

  bdjoptInit ();
  bdjoptSetStr (OPT_M_DIR_MUSIC, "tmp/music");
  diropMakeDir (bdjoptGetStr (OPT_M_DIR_MUSIC));
  bdjvarsdfloadInit ();

  for (int i = 0; i < songparsedatasz; ++i) {
    song = songAlloc ();
    data = mdstrdup (songparsedata [i]);
    songParse (song, data, i);
    mdfree (data);

    snprintf (tbuff, sizeof (tbuff), "%s/%s", bdjoptGetStr (OPT_M_DIR_MUSIC),
        songGetStr (song, TAG_FILE));
    fh = fileopOpen (tbuff, "w");
    fclose (fh);

    rc = songAudioFileExists (song);
    ck_assert_int_eq (rc, 1);
    fileopDelete (tbuff);
    rc = songAudioFileExists (song);
    ck_assert_int_eq (rc, 0);
    songFree (song);
  }

  diropDeleteDir (bdjoptGetStr (OPT_M_DIR_MUSIC));

  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
}
END_TEST

START_TEST(song_display)
{
  song_t      *song = NULL;
  char        *data;
  int         rc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- song_display");
  mdebugSubTag ("song_display");

  bdjvarsdfloadInit ();

  for (int i = 0; i < songparsedatasz; ++i) {
    song = songAlloc ();
    data = mdstrdup (songparsedata [i]);
    songParse (song, data, i);
    mdfree (data);

    /* standard string */
    data = songDisplayString (song, TAG_ARTIST, SONG_NORM);
    ck_assert_str_eq (data, "artist");
    mdfree (data);

    /* converted - these assume the standard data files */

    if (i < 3) {
      data = songDisplayString (song, TAG_GENRE, SONG_NORM);
      ck_assert_str_eq (data, "Classical");
      mdfree (data);
    }

    data = songDisplayString (song, TAG_DANCE, SONG_NORM);
    if (i < 3) {
      ck_assert_str_eq (data, "Waltz");
    } else {
      ck_assert_str_eq (data, "Cha Cha");
    }
    mdfree (data);

    data = songDisplayString (song, TAG_DANCERATING, SONG_NORM);
    ck_assert_str_eq (data, "Good");
    mdfree (data);

    data = songDisplayString (song, TAG_DANCELEVEL, SONG_NORM);
    ck_assert_str_eq (data, "Normal");
    mdfree (data);

    data = songDisplayString (song, TAG_COMPOSER, SONG_NORM);
    if (i < 3) {
      ck_assert_str_eq (data, "composer");
    } else {
      ck_assert_str_eq (data, "");
    }
    mdfree (data);

    if (i < 3) {
      data = songDisplayString (song, TAG_STATUS, SONG_NORM);
      ck_assert_str_eq (data, "New");
      mdfree (data);

      data = songDisplayString (song, TAG_FAVORITE, SONG_NORM);
      rc = strncmp (data, "<span", 5);
      ck_assert_int_eq (rc, 0);
      mdfree (data);

      data = songDisplayString (song, TAG_TAGS, SONG_NORM);
      ck_assert_str_eq (data, "tag1 tag2");
      mdfree (data);
    }
    songFree (song);
  }

  bdjvarsdfloadCleanup ();
}
END_TEST

START_TEST(song_tag_list)
{
  song_t      *song = NULL;
  char        *data;
  const char  *cdata;
  slist_t     *tlist;
  slistidx_t  iteridx;
  char        tbuff [3096];
  const char  *tag;
  datafileconv_t conv;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- song_tag_list");
  mdebugSubTag ("song_tag_list");

  bdjvarsdfloadInit ();

  for (int i = 0; i < songparsedatasz; ++i) {
    double  dval;
    int     rc;

    song = songAlloc ();
    data = mdstrdup (songparsedata [i]);
    songParse (song, data, i);
    mdfree (data);

    tlist = songTagList (song);

    /* this duplicates what the dbWrite() routine does */
    /* to generate the output string for the database */
    snprintf (tbuff, sizeof (tbuff), "%s\n..%s\n", tagdefs [TAG_FILE].tag,
        slistGetStr (tlist, tagdefs [TAG_FILE].tag));
    slistStartIterator (tlist, &iteridx);
    while ((tag = slistIterateKey (tlist, &iteridx)) != NULL) {
      if (strcmp (tag, tagdefs [TAG_FILE].tag) == 0) {
        continue;
      }
      if (strcmp (tag, tagdefs [TAG_LAST_UPDATED].tag) == 0 ||
          strcmp (tag, tagdefs [TAG_RRN].tag) == 0) {
        continue;
      }
      strlcat (tbuff, tag, sizeof (tbuff));
      strlcat (tbuff, "\n", sizeof (tbuff));
      strlcat (tbuff, "..", sizeof (tbuff));
      cdata = slistGetStr (tlist, tag);
      strlcat (tbuff, cdata, sizeof (tbuff));
      strlcat (tbuff, "\n", sizeof (tbuff));
    }
    slistFree (tlist);

    songFree (song);
    song = songAlloc ();
    songParse (song, tbuff, i);

    /* everything (not updated, rrn) should be identical to the original */
    /* not an exhaustive check */

    ck_assert_str_eq (songGetStr (song, TAG_ARTIST), "artist");
    ck_assert_str_eq (songGetStr (song, TAG_ALBUM), "album");
    ck_assert_int_eq (songGetNum (song, TAG_DANCERATING), 2);
    ck_assert_int_eq (songGetNum (song, TAG_DANCELEVEL), 1);
    if (i < 3) {
      ck_assert_int_eq (songGetNum (song, TAG_DISCNUMBER), 1);
      ck_assert_int_eq (songGetNum (song, TAG_TRACKNUMBER), 5);
      ck_assert_int_eq (songGetNum (song, TAG_TRACKTOTAL), 10);
      if (i == 0) {
        ck_assert_int_eq (songGetNum (song, TAG_ADJUSTFLAGS), 7);
      }
      if (i == 1) {
        ck_assert_int_eq (songGetNum (song, TAG_ADJUSTFLAGS), 3);
      }
      if (i == 2) {
        ck_assert_int_eq (songGetNum (song, TAG_ADJUSTFLAGS), 0);
      }
      dval = songGetDouble (song, TAG_VOLUMEADJUSTPERC);
      rc = dval == 4.4 || dval == -4.4;
      ck_assert_int_eq (rc, 1);

      tlist = songGetList (song, TAG_TAGS);
      slistStartIterator (tlist, &iteridx);
      cdata = slistIterateKey (tlist, &iteridx);
      ck_assert_str_eq (cdata, "tag1");
      cdata = slistIterateKey (tlist, &iteridx);
      ck_assert_str_eq (cdata, "tag2");
      /* converted - these assume the standard data files */
      ck_assert_int_eq (songGetNum (song, TAG_GENRE), 2);
      ck_assert_int_eq (songGetNum (song, TAG_STATUS), 0);
      ck_assert_int_eq (songGetNum (song, TAG_DANCE), 11);

      conv.invt = VALUE_STR;
      conv.str = "bluestar";
      songFavoriteConv (&conv);
      ck_assert_int_eq (songGetNum (song, TAG_FAVORITE), conv.num);

      ck_assert_str_eq (songGetStr (song, TAG_COMPOSER), "composer");
    }
    if (i == 3) {
      ck_assert_str_eq (songGetStr (song, TAG_COMPOSER), "");
    }

    songFree (song);
  }

  bdjvarsdfloadCleanup ();
}
END_TEST

Suite *
song_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("song");
  tc = tcase_create ("song");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, NULL);
  tcase_add_test (tc, song_alloc);
  tcase_add_test (tc, song_parse);
  tcase_add_test (tc, song_parse_get);
  tcase_add_test (tc, song_parse_set);
  tcase_add_test (tc, song_audio_file);
  tcase_add_test (tc, song_display);
  tcase_add_test (tc, song_tag_list);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
