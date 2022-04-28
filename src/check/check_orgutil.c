#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>

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
  "Waltz/The First Waltz.flac",
  NULL,
};
char *testpathb [] = {
  "Ballroom_Dance/Various/White_Owl/01.松居慶子_-_White_Owl.mp3",
  "Unknown/Various/The_Ultimate_Ballroom_Album_3/2-03.Starlight_(Waltz,_29mpm).mp3"
  "Classical/Secret_Garden/Dreamcatcher/04.Dreamcatcher.mp3",
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
  char      *path = "{%DANCE%/}{%TITLE%}";
  char      *val;

  org = orgAlloc (path);
  ck_assert_ptr_nonnull (org);
  while (testpatha [i] != NULL) {
    val = orgGetFromPath (org, testpatha [i], TAG_DANCE);
    ck_assert_str_eq (val, "Waltz");
    val = orgGetFromPath (org, testpatha [i], TAG_TITLE);
    ck_assert_ptr_nonnull (org);
    ++i;
  }
  orgFree (org);

  path = "{%GENRE%/}{%ALBUMARTIST%/}{%ALBUM%/}{%DISC%-}{%TRACKNUMBER0%.}{%TITLE%}";

  org = orgAlloc (path);
  ck_assert_ptr_nonnull (org);
  while (testpathb [i] != NULL) {
    val = orgGetFromPath (org, testpathb [i], TAG_TITLE);
    if (i == 0) {
      ck_assert_str_eq (val, "松居慶子_-_White_Owl");
    }
    if (i == 1) {
      ck_assert_str_eq (val, "Starlight_(Waltz,_29mpm)");
    }
    if (i == 2) {
      ck_assert_str_eq (val, "Dreamcatcher");
    }
    val = orgGetFromPath (org, testpathb [i], TAG_DISCNUMBER);
    if (i == 1) {
      ck_assert_str_eq (val, "2");
    }
    val = orgGetFromPath (org, testpathb [i], TAG_TRACKNUMBER);
    if (i == 0) {
      ck_assert_str_eq (val, "1");
    }
    if (i == 1) {
      ck_assert_str_eq (val, "3");
    }
    if (i == 2) {
      ck_assert_str_eq (val, "4");
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
  tc = tcase_create ("orgutils");
  tcase_add_test (tc, orgutil_parse);
  tcase_add_test (tc, orgutil_regex);
  suite_add_tcase (s, tc);
  return s;
}

