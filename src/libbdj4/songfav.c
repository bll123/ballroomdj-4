#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "datafile.h"
#include "songfav.h"

static datafilekey_t favoritedfkeys [SONG_FAVORITE_MAX] = {
  { "",           SONG_FAVORITE_NONE,     VALUE_STR, NULL, -1 },
  { "bluestar",   SONG_FAVORITE_BLUE,     VALUE_STR, NULL, -1 },
  { "brokenheart",SONG_FAVORITE_BROKEN_HEART, VALUE_STR, NULL, -1 },
  { "greenstar",  SONG_FAVORITE_GREEN,    VALUE_STR, NULL, -1 },
  { "orangestar", SONG_FAVORITE_ORANGE,   VALUE_STR, NULL, -1 },
  { "pinkheart",  SONG_FAVORITE_PINK_HEART, VALUE_STR, NULL, -1 },
  { "purplestar", SONG_FAVORITE_PURPLE,   VALUE_STR, NULL, -1 },
  { "redstar",    SONG_FAVORITE_RED,      VALUE_STR, NULL, -1 },
};

static songfavoriteinfo_t songfavoriteinfo [SONG_FAVORITE_MAX] = {
  { SONG_FAVORITE_NONE, "\xE2\x98\x86", "", NULL },
  { SONG_FAVORITE_RED, "\xE2\x98\x85", "#9b3128", NULL },
  { SONG_FAVORITE_ORANGE, "\xE2\x98\x85", "#c26a1a", NULL },
  { SONG_FAVORITE_GREEN, "\xE2\x98\x85", "#007223", NULL },
  { SONG_FAVORITE_BLUE, "\xE2\x98\x85", "#2f2ad7", NULL },
  { SONG_FAVORITE_PURPLE, "\xE2\x98\x85", "#901ba3", NULL },
  { SONG_FAVORITE_PINK_HEART, "\xE2\x99\xa5", "#ee00e1", NULL },
  { SONG_FAVORITE_BROKEN_HEART, "\xF0\x9F\x92\x94", "#6d2828", NULL },
};

void
songFavoriteInit (void)
{
  char  *col;
  char  tbuff [100];

  for (int i = 0; i < SONG_FAVORITE_MAX; ++i) {
    if (i == 0) {
      col = "#ffffff";
    } else {
      col = songfavoriteinfo [i].color;
    }
    snprintf (tbuff, sizeof (tbuff), "<span color=\"%s\">%s</span>",
        col, songfavoriteinfo [i].dispStr);
    songfavoriteinfo [i].spanStr = strdup (tbuff);
  }
}

void
songFavoriteCleanup (void)
{
  for (int i = 0; i < SONG_FAVORITE_MAX; ++i) {
    if (songfavoriteinfo [i].spanStr != NULL) {
      free (songfavoriteinfo [i].spanStr);
      songfavoriteinfo [i].spanStr = NULL;
    }
  }
}

void
songConvFavorite (datafileconv_t *conv)
{
  nlistidx_t       idx;

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;
    if (conv->str == NULL || strcmp (conv->str, "") == 0) {
      conv->num = SONG_FAVORITE_NONE;
      return;
    }
    idx = dfkeyBinarySearch (favoritedfkeys, SONG_FAVORITE_MAX, conv->str);
    if (idx < 0) {
      conv->num = SONG_FAVORITE_NONE;
    } else {
      conv->num = favoritedfkeys [idx].itemkey;
    }
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    idx = conv->num;
    if (idx < 0 || idx >= SONG_FAVORITE_MAX) {
      idx = SONG_FAVORITE_NONE;
    }
    for (int i = 0; i < SONG_FAVORITE_MAX; ++i) {
      if (favoritedfkeys [i].itemkey == idx) {
        conv->str = favoritedfkeys [i].name;
        break;
      }
    }
  }
}

songfavoriteinfo_t *
songFavoriteGet (int value)
{
  return &songfavoriteinfo [value];
}

