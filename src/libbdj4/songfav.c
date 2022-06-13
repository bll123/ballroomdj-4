#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "datafile.h"
#include "songfav.h"

static datafilekey_t favoritedfkeys [SONG_FAVORITE_MAX] = {
  { "bluestar",   SONG_FAVORITE_BLUE,     VALUE_STR, NULL, -1 },
  { "greenstar",  SONG_FAVORITE_GREEN,    VALUE_STR, NULL, -1 },
  { "none",       SONG_FAVORITE_NONE,     VALUE_STR, NULL, -1 },
  { "orangestar", SONG_FAVORITE_ORANGE,   VALUE_STR, NULL, -1 },
  { "purplestar", SONG_FAVORITE_PURPLE,   VALUE_STR, NULL, -1 },
  { "redstar",    SONG_FAVORITE_RED,      VALUE_STR, NULL, -1 },
};

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
    conv->str = favoritedfkeys [idx].name;
  }
}

