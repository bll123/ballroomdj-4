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

#include "bdjvarsdfload.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"
#include "orgutil.h"
#include "sysvars.h"

char *orgpaths [] = {
  "{%ALBUM%/}{%ALBUMARTIST%/}{%TRACKNUMBER% - }{%TITLE%}",
  "{%ALBUM%/}{%ARTIST%/}{%TRACKNUMBER% - }{%TITLE%}",
  "{%ALBUMARTIST% - }{%TITLE%}",
  "{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER%.}{%TITLE%}",
  "{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER0%.}{%TITLE%}",
  "{%ALBUMARTIST%/}{%TITLE%}",
  "{%ARTIST% - }{%TITLE%}",
  "{%ARTIST%/}{%TITLE%}",
  "{%BYPASS%}{%DANCE% - }{%TITLE%}",
  "{%DANCE% - }{%TITLE%}",
  "{%DANCE%/}{%ARTIST% - }{%TITLE%}",
  "{%DANCE%/}{%TITLE%}",
  "{%GENRE%/}{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER0%.}{%TITLE%}",
  "{%GENRE%/}{%COMPOSER%/}{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER0%.}{%TITLE%}",
  "{%TITLE%}",
  NULL,
};

char *testpatha [] = {
  "Waltz/The Last Waltz.mp3",
  "Waltz/The Last Waltz.flac",
  NULL,
};
char *testpathb [] = {
  "Waltz/Artist - The Last Waltz.mp3",
  "Waltz/Artist - The Last Waltz.flac",
  NULL,
};
char *testpathc [] = {
  "BypassA/Ballroom_Dance/Various/White_Owl/01.松居慶子_-_White_Owl.mp3",
  "BypassA/Unknown/Various/The_Ultimate_Ballroom_Album_3/2-03.Starlight_(Waltz,_29mpm).mp3",
  "BypassB/Classical/Secret_Garden/Dreamcatcher/04.Dreamcatcher.mp3",
  "BypassA/Unknown/Various/The_Ultimate_Ballroom_Album_3/03.Starlight_(Waltz,_29mpm).mp3",
  "BypassA/Unknown/Various/The_Ultimate_Ballroom_Album_3/Starlight_(Waltz,_29mpm).mp3",
  "/home/music/m/BypassB/Anime/Evan_Call/VIOLET_EVERGARDEN_Automemories/01.Theme_of_Violet_Evergarden.mp3",
  NULL,
};
char *testpathd [] = {
  "松居慶子_-_White_Owl.mp3",
  "Starlight_(Waltz,_29mpm).mp3",
  "Dreamcatcher.mp3",
  "Starlight_(Waltz,_29mpm).mp3",
  "Starlight_(Waltz,_29mpm).mp3",
  "Theme_of_Violet_Evergarden.mp3",
  NULL,
};

/* note that to retrieve the bypass data, the filename must */
/* be parsed using the old path. */
/* for this test data, the old path will be {%BYPASS%/}{%TITLE} */
char *testsongdata [] = {
  /* unknown genre */
  "FILE\n..bypass2/none1.mp3\nDISC\n..1\nTRACKNUMBER\n..1\n"
      "ALBUM\n..Smooth\nALBUMARTIST\n..Santana\n"
      "ARTIST\n..Santana\nDANCE\n..Cha Cha\nTITLE\n..Smooth [128]\n"
      "GENRE\n..Jazz\nCOMPOSER\n..Composer 1\n",
  /* no genre */
  "FILE\n..bypass1/none2.mp3\nDISC\n..1\nTRACKNUMBER\n..2\n"
      "ALBUM\n..The Ultimate Latin Album 4: Latin Eyes\nALBUMARTIST\n..WRD\n"
      "ARTIST\n..Gizelle D'Cole\nDANCE\n..Rumba\nTITLE\n..Asi\n"
      "GENRE\n..\nCOMPOSER\n..Composer 2\n",
  "FILE\n..bypass0/none3.mp3\nDISC\n..1\nTRACKNUMBER\n..3\n"
      "ALBUM\n..Ballroom Stars 6\nALBUMARTIST\n..Various Artists\n"
      "ARTIST\n..Léa\nDANCE\n..Waltz\nTITLE\n..Je Vole! (from 'La Famille Bélier')\n"
      "GENRE\n..Ballroom Dance\nCOMPOSER\n..Composer 3\n",
  /* empty albumartist */
  "FILE\n..bypassA/none4.mp3\nDISC\n..2\nTRACKNUMBER\n..4\n"
      "ALBUM\n..The Ultimate Latin Album 9: Footloose\nALBUMARTIST\n..\n"
      "ARTIST\n..Gloria Estefan\nDANCE\n..Rumba\nTITLE\n..Me Voy\n"
      "GENRE\n..Jazz\nCOMPOSER\n..Composer 4\n",
  /* no bypass directory */
  "FILE\n..none5.mp3\nDISC\n..3\nTRACKNUMBER\n..5\n"
      "ALBUM\n..Album 5\nALBUMARTIST\n..AlbumArtist 5\n"
      "ARTIST\n..Artist 5\nDANCE\n..Rumba\nTITLE\n..こんにちは-ja\n"
      "GENRE\n..Jazz\nCOMPOSER\n..Composer 5\n",
  "FILE\n..bypassC/none6.mp3\nDISC\n..3\nTRACKNUMBER\n..6\n"
      "ALBUM\n..Album 6\nALBUMARTIST\n..AlbumArtist 6\n"
      "ARTIST\n..Artist 6\nDANCE\n..Rumba\nTITLE\n..Ne_Русский_Шторм-ru\n"
      "GENRE\n..Rock\nCOMPOSER\n..Composer 6\n",
  "FILE\n..bypassD/none7.mp3\nDISC\n..3\nTRACKNUMBER\n..7\n"
      "ALBUM\n..Album 7\nALBUMARTIST\n..AlbumArtist 7\n"
      "ARTIST\n..Artist 7\nDANCE\n..Cha Cha\nTITLE\n..Title 7\n"
      "GENRE\n..Classical\nCOMPOSER\n..Composer 7\n",
  /* dot at end of album artist, on windows this must not be present */
  /* if the album artist is a directory name */
  "FILE\n..bypassE/none8.mp3\nDISC\n..3\nTRACKNUMBER\n..8\n"
      "ALBUM\n..Album 8\nALBUMARTIST\n..AlbumArtist 8.\n"
      "ARTIST\n..Artist 8\nDANCE\n..Rumba\nTITLE\n..Ne_Русский_Шторм-ru\n"
      "GENRE\n..Rock\nCOMPOSER\n..Composer 8\n",
  NULL,
};

typedef struct {
  const char    *orgpath;
  const char    *results [20];
  char          cleanresults [20][200];
} testsong_t;

testsong_t testsongresults [] = {
  {
    "{%BYPASS%/}{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER0%.}{%TITLE%}",
    {
      "bypass2/Santana/Smooth/01-001.Smooth [128].mp3",
      "bypass1/WRD/The Ultimate Latin Album 4: Latin Eyes/01-002.Asi.mp3",
      "bypass0/Various Artists/Ballroom Stars 6/01-003.Je Vole! (from La Famille Bélier).mp3",
      /* empty album artist is replaced with the artist */
      "bypassA/Gloria Estefan/The Ultimate Latin Album 9: Footloose/02-004.Me Voy.mp3",
      /* no bypass dir */
      "AlbumArtist 5/Album 5/03-005.こんにちは-ja.mp3",
      "bypassC/AlbumArtist 6/Album 6/03-006.Ne_Русский_Шторм-ru.mp3",
      "bypassD/AlbumArtist 7/Album 7/03-007.Title 7.mp3",
      "bypassE/AlbumArtist 8./Album 8/03-008.Ne_Русский_Шторм-ru.mp3",
      NULL,
    },
    { "", },
  },
  {
    "{%DANCE%/}{%ARTIST% - }{%TITLE%}",
    {
      "Cha Cha/Santana - Smooth [128].mp3",
      "Rumba/Gizelle DCole - Asi.mp3",
      "Waltz/Léa - Je Vole! (from La Famille Bélier).mp3",
      "Rumba/Gloria Estefan - Me Voy.mp3",
      "Rumba/Artist 5 - こんにちは-ja.mp3",
      "Rumba/Artist 6 - Ne_Русский_Шторм-ru.mp3",
      "Cha Cha/Artist 7 - Title 7.mp3",
      "Rumba/Artist 8 - Ne_Русский_Шторм-ru.mp3",
      NULL,
    },
    { "", },
  },
  {
    "{%GENRE%/}{%COMPOSER%/}{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER0%.}{%TITLE%}",
    {
      "Jazz/Santana/Smooth/01-001.Smooth [128].mp3",
      /* no genre */
      "WRD/The Ultimate Latin Album 4: Latin Eyes/01-002.Asi.mp3",
      "Ballroom Dance/Various Artists/Ballroom Stars 6/01-003.Je Vole! (from La Famille Bélier).mp3",
      "Jazz/Gloria Estefan/The Ultimate Latin Album 9: Footloose/02-004.Me Voy.mp3",
      "Jazz/AlbumArtist 5/Album 5/03-005.こんにちは-ja.mp3",
      "Rock/AlbumArtist 6/Album 6/03-006.Ne_Русский_Шторм-ru.mp3",
      "Classical/Composer 7/AlbumArtist 7/Album 7/03-007.Title 7.mp3",
      "Rock/AlbumArtist 8./Album 8/03-008.Ne_Русский_Шторм-ru.mp3",
      NULL,
    },
    { "", },
  },
  {
    NULL,
    { NULL },
    { "", },
  },
};

static void
setup (void)
{
  bdjvarsdfloadInit ();
}

static void
teardown (void)
{
  bdjvarsdfloadCleanup ();
}

START_TEST(orgutil_parse)
{
  int       i = 0;
  org_t     *org;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- orgutil_parse");
  mdebugSubTag ("orgutil_parse");

  while (orgpaths [i] != NULL) {
    org = orgAlloc (orgpaths[i]);
    ck_assert_ptr_nonnull (org);
    orgFree (org);
    ++i;
  }
}
END_TEST

START_TEST(orgutil_regex)
{
  int         i = 0;
  org_t       *org;
  const char  *path;
  const char  *val;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- orgutil_regex");
  mdebugSubTag ("orgutil_regex");

  path = "{%DANCE%/}{%TITLE%}";

  org = orgAlloc (path);
  ck_assert_ptr_nonnull (org);
  i = 0;
  while (testpatha [i] != NULL) {
    val = orgGetFromPath (org, testpatha [i], TAG_DANCE);
    ck_assert_str_eq (val, "Waltz");
    val = orgGetFromPath (org, testpatha [i], TAG_TITLE);
    ck_assert_str_eq (val, "The Last Waltz");
    ++i;
  }
  orgFree (org);

  path = "{%DANCE%/}{%ARTIST% - }{%TITLE%}";

  org = orgAlloc (path);
  ck_assert_ptr_nonnull (org);
  i = 0;
  while (testpathb [i] != NULL) {
    val = orgGetFromPath (org, testpathb [i], TAG_DANCE);
    ck_assert_str_eq (val, "Waltz");
    val = orgGetFromPath (org, testpathb [i], TAG_ARTIST);
    ck_assert_str_eq (val, "Artist");
    val = orgGetFromPath (org, testpathb [i], TAG_TITLE);
    ck_assert_str_eq (val, "The Last Waltz");
    ++i;
  }
  orgFree (org);

  path = "{%BYPASS%/}{%GENRE%/}{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER0%.}{%TITLE%}";

  org = orgAlloc (path);
  ck_assert_ptr_nonnull (org);
  i = 0;
  while (testpathc [i] != NULL) {
    val = orgGetFromPath (org, testpathc [i], (tagdefkey_t) ORG_TAG_BYPASS);
    if (i == 0) {
      ck_assert_str_eq (val, "BypassA");
    }
    if (i == 5) {
      ck_assert_str_eq (val, "BypassB");
    }
    val = orgGetFromPath (org, testpathc [i], TAG_GENRE);
    if (i == 5) {
      ck_assert_str_eq (val, "Anime");
    }
    val = orgGetFromPath (org, testpathc [i], TAG_GENRE);
    if (i == 5) {
      ck_assert_str_eq (val, "Anime");
    }
    val = orgGetFromPath (org, testpathc [i], TAG_ALBUMARTIST);
    if (i == 5) {
      ck_assert_str_eq (val, "Evan_Call");
    }
    val = orgGetFromPath (org, testpathc [i], TAG_ALBUM);
    if (i == 5) {
      ck_assert_str_eq (val, "VIOLET_EVERGARDEN_Automemories");
    }
    val = orgGetFromPath (org, testpathc [i], TAG_TITLE);
    if (i == 0) {
      ck_assert_str_eq (val, "松居慶子_-_White_Owl");
    }
    if (i == 1) {
      ck_assert_str_eq (val, "Starlight_(Waltz,_29mpm)");
    }
    if (i == 2) {
      ck_assert_str_eq (val, "Dreamcatcher");
    }
    if (i == 3 || i == 4) {
      ck_assert_str_eq (val, "Starlight_(Waltz,_29mpm)");
    }
    if (i == 5) {
      ck_assert_str_eq (val, "Theme_of_Violet_Evergarden");
    }
    val = orgGetFromPath (org, testpathc [i], TAG_DISCNUMBER);
    if (i == 1) {
      ck_assert_int_eq (atoi(val), 2);
    }
    val = orgGetFromPath (org, testpathc [i], TAG_TRACKNUMBER);
    if (i == 0) {
      ck_assert_int_eq (atoi(val), 1);
    }
    if (i == 1) {
      ck_assert_int_eq (atoi(val), 3);
    }
    if (i == 2) {
      ck_assert_int_eq (atoi(val), 4);
    }
    if (i == 3) {
      ck_assert_int_eq (atoi(val), 3);
    }
    if (i == 4) {
      ck_assert_int_eq (atoi(val), 1);
    }
    if (i == 5) {
      ck_assert_int_eq (atoi(val), 1);
    }
    ++i;
  }
  orgFree (org);

  path = "{%TITLE%}";

  org = orgAlloc (path);
  ck_assert_ptr_nonnull (org);
  i = 0;
  while (testpathd [i] != NULL) {
    val = orgGetFromPath (org, testpathd [i], TAG_TITLE);
    if (i == 0) {
      ck_assert_str_eq (val, "松居慶子_-_White_Owl");
    }
    if (i == 1 || i == 3 || i == 4) {
      ck_assert_str_eq (val, "Starlight_(Waltz,_29mpm)");
    }
    if (i == 2) {
      ck_assert_str_eq (val, "Dreamcatcher");
    }
    if (i == 5) {
      ck_assert_str_eq (val, "Theme_of_Violet_Evergarden");
    }
    ++i;
  }

  orgFree (org);
}
END_TEST

START_TEST(orgutil_makepath)
{
  org_t     *orgbp;
  org_t     *org;
  int       ri;
  int       i;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- orgutil_makepath");
  mdebugSubTag ("orgutil_makepath");

  ri = 0;
  while (testsongresults [ri].orgpath != NULL) {
    const char  *res;
    i = 0;
    while ((res = testsongresults [ri].results [i]) != NULL) {
      const char  *p;
      char        *o;

      /* this is not correct, as it does not handle multi-byte sequences */
      /* properly */
      p = res;
      o = testsongresults [ri].cleanresults [i];
      while (*p) {
        if (*p == '*' || *p == '&' || *p == '|' || *p == '<' || *p == '>' || *p == '?' || *p == '\'' || *p == '"') {
          ++p;
          continue;
        }
        if (isWindows () && (*p == '(' || *p == ')' || *p == ':' || *p == '^')) {
          ++p;
          continue;
        }
        if (! (isWindows ()) && (*p == '[' || *p == ']')) {
          ++p;
          continue;
        }
        *o = *p;
        ++o;
        ++p;
      }
      *o = '\0';
      ++i;
    }
    ++ri;
  }

  orgbp = orgAlloc ("{%BYPASS%/}{%TITLE%}");
  ck_assert_ptr_nonnull (orgbp);

  ri = 0;
  while (testsongresults [ri].orgpath != NULL) {
    i = 0;
    // fprintf (stderr, "ri: %d %s\n", ri, testsongresults [ri].orgpath);
    org = orgAlloc (testsongresults [ri].orgpath);
    ck_assert_ptr_nonnull (org);

    while (testsongresults [ri].results [i] != NULL) {
      char        *tdata;
      char        *disp;
      song_t      *song;
      const char  *bypass;

      // fprintf (stderr, "i: %d %s\n", i, testsongresults [ri].cleanresults [i]);
      tdata = mdstrdup (testsongdata [i]);
      song = songAlloc ();
      songParse (song, tdata, 0);
      bypass = "";
      if (strstr (testsongresults [ri].orgpath, "BYPASS") != NULL) {
        bypass = orgGetFromPath (orgbp, songGetStr (song, TAG_URI),
            (tagdefkey_t) ORG_TAG_BYPASS);
        // fprintf (stderr, "ri: %d i: %d bypass: %s\n", ri, i, bypass);
      }
      disp = orgMakeSongPath (org, song, bypass);
      ck_assert_str_eq (testsongresults [ri].cleanresults [i], disp);
      songFree (song);
      mdfree (tdata);
      mdfree (disp);
      ++i;
    }

    orgFree (org);
    ++ri;
  }

  orgFree (orgbp);
}
END_TEST

Suite *
orgutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("orgutil");
  tc = tcase_create ("orgutil");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, orgutil_parse);
  tcase_add_test (tc, orgutil_regex);
  tcase_add_test (tc, orgutil_makepath);
  suite_add_tcase (s, tc);
  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
