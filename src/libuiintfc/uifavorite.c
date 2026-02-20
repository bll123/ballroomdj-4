/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "mdebug.h"
#include "musicdb.h"
#include "song.h"
#include "songfav.h"
#include "ui.h"
#include "uifavorite.h"
#include "uisbtext.h"
#include "uiutils.h"

typedef struct uifavorite {
  uisbtext_t  *sb;
  songfav_t   *songfav;
} uifavorite_t;

static const char *uifavoriteCBHandler (void *udata, int idx);

uifavorite_t *
uifavoriteSpinboxCreate (uiwcont_t *boxp)
{
  uifavorite_t  *uifavorite;

  uifavorite = mdmalloc (sizeof (uifavorite_t));
  uifavorite->songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
  uifavorite->sb = uisbtextCreate (boxp);

  uiutilsAddFavoriteClasses ();

  uisbtextSetDisplayCallback (uifavorite->sb, uifavoriteCBHandler, uifavorite);
  uisbtextSetCount (uifavorite->sb, songFavoriteGetCount (uifavorite->songfav));
  uisbtextSetWidth (uifavorite->sb, 2);

  return uifavorite;
}


void
uifavoriteFree (uifavorite_t *uifavorite)
{
  if (uifavorite == NULL) {
    return;
  }

  uisbtextFree (uifavorite->sb);
  mdfree (uifavorite);
}

int
uifavoriteGetValue (uifavorite_t *uifavorite)
{
  int   idx;

  if (uifavorite == NULL) {
    return 0;
  }

  idx = uisbtextGetValue (uifavorite->sb);
  return idx;
}

void
uifavoriteSetValue (uifavorite_t *uifavorite, int value)
{
  if (uifavorite == NULL) {
    return;
  }

  if (value < 0) {
    value = 0;
  }

  uisbtextSetValue (uifavorite->sb, value);
}

void
uifavoriteSetState (uifavorite_t *uifavorite, int state)
{
  if (uifavorite == NULL || uifavorite->sb == NULL) {
    return;
  }
  uisbtextSetState (uifavorite->sb, state);
}

void
uifavoriteSetChangedCallback (uifavorite_t *uifavorite, callback_t *cb)
{
  if (uifavorite == NULL || cb == NULL || uifavorite->sb == NULL) {
    return;
  }

  uisbtextSetChangeCallback (uifavorite->sb, cb);
}

/* internal routines */

static const char *
uifavoriteCBHandler (void *udata, int idx)
{
  uifavorite_t  *uifavorite = udata;
  const char    *name;
  int           count;

  count = songFavoriteGetCount (uifavorite->songfav);
  for (int i = 0; i < count; ++i) {
    name = songFavoriteGetStr (uifavorite->songfav, i, SONGFAV_NAME);
    if (i == idx) {
      uisbtextAddClass (uifavorite->sb, name);
    } else {
      uisbtextRemoveClass (uifavorite->sb, name);
    }
  }

  return songFavoriteGetStr (uifavorite->songfav, idx, SONGFAV_DISPLAY);
}
