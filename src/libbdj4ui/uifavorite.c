/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
#include "uiutils.h"

typedef struct uifavorite {
  uiwcont_t   *spinbox;
  songfav_t   *songfav;
} uifavorite_t;

static const char *uifavoriteFavoriteGet (void *udata, int idx);

uifavorite_t *
uifavoriteSpinboxCreate (uiwcont_t *boxp)
{
  uifavorite_t  *uifavorite;

  uifavorite = mdmalloc (sizeof (uifavorite_t));
  uifavorite->songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
  uifavorite->spinbox = uiSpinboxTextCreate (uifavorite);

  uiutilsAddFavoriteClasses ();

  uiSpinboxTextSet (uifavorite->spinbox, 0,
      songFavoriteGetCount (uifavorite->songfav),
      2, NULL, NULL, uifavoriteFavoriteGet);

  uiBoxPackStart (boxp, uifavorite->spinbox);

  return uifavorite;
}


void
uifavoriteFree (uifavorite_t *uifavorite)
{
  if (uifavorite == NULL) {
    return;
  }

  uiwcontFree (uifavorite->spinbox);
  mdfree (uifavorite);
}

int
uifavoriteGetValue (uifavorite_t *uifavorite)
{
  int   idx;

  if (uifavorite == NULL) {
    return 0;
  }

  idx = uiSpinboxTextGetValue (uifavorite->spinbox);
  return idx;
}

void
uifavoriteSetValue (uifavorite_t *uifavorite, int value)
{
  if (uifavorite == NULL) {
    return;
  }

  uiSpinboxTextSetValue (uifavorite->spinbox, value);
}

void
uifavoriteSetState (uifavorite_t *uifavorite, int state)
{
  if (uifavorite == NULL || uifavorite->spinbox == NULL) {
    return;
  }
  uiSpinboxSetState (uifavorite->spinbox, state);
}

void
uifavoriteSetChangedCallback (uifavorite_t *uifavorite, callback_t *cb)
{
  uiSpinboxTextSetValueChangedCallback (uifavorite->spinbox, cb);
}

/* internal routines */

static const char *
uifavoriteFavoriteGet (void *udata, int idx)
{
  uifavorite_t  *uifavorite = udata;
  const char    *name;
  int           count;

  count = songFavoriteGetCount (uifavorite->songfav);
  for (int i = 0; i < count; ++i) {
    name = songFavoriteGetStr (uifavorite->songfav, i, SONGFAV_NAME);
    if (i == idx) {
      uiWidgetAddClass (uifavorite->spinbox, name);
    } else {
      uiWidgetRemoveClass (uifavorite->spinbox, name);
    }
  }
  return songFavoriteGetStr (uifavorite->songfav, idx, SONGFAV_DISPLAY);
}

