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

#include "orgutil.h"
#include "check_bdj.h"

char *orgpaths [] = {
  "{%ALBUM%/}{%ALBUMARTIST%/}{%TRACKNUMBER% - }{%TITLE%}",
  "{%ALBUM%/}{%ARTIST%/}{%TRACKNUMBER% - }{%TITLE%}",
  "{%ALBUMARTIST% - }{%TITLE%}",
  "{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER%.}{%TITLE%}",
  "{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER0%.}{%TITLE%}",
  "{%ALBUMARTIST%/}{%TITLE%}",
  "{%ARTIST% - }{%TITLE%}",
  "{%ARTIST%/}{%TITLE%}",
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
  "Ballroom_Dance/Various/White_Owl/01.松居慶子_-_White_Owl.mp3",
  "Unknown/Various/The_Ultimate_Ballroom_Album_3/2-03.Starlight_(Waltz,_29mpm).mp3",
  "Classical/Secret_Garden/Dreamcatcher/04.Dreamcatcher.mp3",
  "Unknown/Various/The_Ultimate_Ballroom_Album_3/03.Starlight_(Waltz,_29mpm).mp3",
  "Unknown/Various/The_Ultimate_Ballroom_Album_3/Starlight_(Waltz,_29mpm).mp3",
  "/home/music/m/Anime/Evan_Call/VIOLET_EVERGARDEN_Automemories/01.Theme_of_Violet_Evergarden.mp3",
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

START_TEST(orgutil_parse)
{
  int       i = 0;
  org_t     *org;

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
  int       i = 0;
  org_t     *org;
  char      *path;
  char      *val;

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

  path = "{%GENRE%/}{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER0%.}{%TITLE%}";

  org = orgAlloc (path);
  ck_assert_ptr_nonnull (org);
  i = 0;
  while (testpathc [i] != NULL) {
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

Suite *
orgutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("orgutil Suite");
  tc = tcase_create ("orgutil");
  tcase_add_test (tc, orgutil_parse);
  tcase_add_test (tc, orgutil_regex);
  suite_add_tcase (s, tc);
  return s;
}
