/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "musicdb.h"
#include "song.h"
#include "songfav.h"
#include "ui.h"
#include "uifavorite.h"

typedef struct uifavorite {
  uispinbox_t   *spinbox;
  songfav_t     *songfav;
  char          textcolor [40];
} uifavorite_t;

static const char *uifavoriteFavoriteGet (void *udata, int idx);
static void uifavoriteSetFavoriteForeground (uifavorite_t *uifavorite, const char *color);

uifavorite_t *
uifavoriteSpinboxCreate (UIWidget *boxp)
{
  uifavorite_t  *uifavorite;
  UIWidget      *uispinboxp;

  uifavorite = malloc (sizeof (uifavorite_t));
  uifavorite->songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
  uifavorite->spinbox = uiSpinboxInit ();
  uispinboxp = uiSpinboxGetUIWidget (uifavorite->spinbox);
  uiSpinboxTextCreate (uifavorite->spinbox, uifavorite);
  /* get the foreground color once only before anything has been changed */
  uiGetForegroundColor (uispinboxp,
      uifavorite->textcolor, sizeof (uifavorite->textcolor));

  uiSpinboxTextSet (uifavorite->spinbox, 0,
      songFavoriteGetCount (uifavorite->songfav),
      2, NULL, NULL, uifavoriteFavoriteGet);

  uiBoxPackStart (boxp, uispinboxp);

  return uifavorite;
}


void
uifavoriteFree (uifavorite_t *uifavorite)
{
  if (uifavorite != NULL) {
    uiSpinboxFree (uifavorite->spinbox);
    free (uifavorite);
  }
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
uifavoriteDisable (uifavorite_t *uifavorite)
{
  if (uifavorite == NULL || uifavorite->spinbox == NULL) {
    return;
  }
  uiSpinboxTextDisable (uifavorite->spinbox);
}

void
uifavoriteEnable (uifavorite_t *uifavorite)
{
  if (uifavorite == NULL || uifavorite->spinbox == NULL) {
    return;
  }
  uiSpinboxTextEnable (uifavorite->spinbox);
}

/* internal routines */

static const char *
uifavoriteFavoriteGet (void *udata, int idx)
{
  uifavorite_t  *uifavorite = udata;
  const char    *color;

  color = songFavoriteGetStr (uifavorite->songfav, idx, SONGFAV_COLOR);
  uifavoriteSetFavoriteForeground (uifavorite, color);
  return songFavoriteGetStr (uifavorite->songfav, idx, SONGFAV_DISPLAY);
}

static void
uifavoriteSetFavoriteForeground (uifavorite_t *uifavorite, const char *color)
{
  if (color != NULL && strcmp (color, "") != 0) {
    uiSpinboxSetColor (uifavorite->spinbox, color);
  } else {
    uiSpinboxSetColor (uifavorite->spinbox, uifavorite->textcolor);
  }
}

