/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "slist.h"
#include "songfav.h"

/* must be sorted in ascii order */
static datafilekey_t songfavdfkeys [SONGFAV_KEY_MAX] = {
  { "COLOR",    SONGFAV_COLOR,    VALUE_STR, NULL, DF_NORM },
  { "DISPLAY",  SONGFAV_DISPLAY,  VALUE_STR, NULL, DF_NORM },
  { "NAME",     SONGFAV_NAME,     VALUE_STR, NULL, DF_NORM },
  { "USERSEL",  SONGFAV_USERSEL,  VALUE_NUM, convBoolean, DF_NORM },
};

typedef struct songfav {
  datafile_t    *df;
  ilist_t       *songfavList;
  slist_t       *songfavLookup;
  /* count is the number of user selectable favorites */
  int           count;
} songfav_t;

songfav_t *
songFavoriteAlloc (void)
{
  char        fname [MAXPATHLEN];
  songfav_t   *songfav;
  ilistidx_t  iteridx;
  ilistidx_t  key;

  pathbldMakePath (fname, sizeof (fname), "favorites",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: favorites: missing %s", fname);
    return NULL;
  }

  songfav = mdmalloc (sizeof (songfav_t));

  songfav->df = datafileAllocParse ("favorites", DFTYPE_INDIRECT, fname,
        songfavdfkeys, SONGFAV_KEY_MAX, DF_NO_OFFSET, NULL);
  songfav->songfavList = datafileGetList (songfav->df);
  /* temporarily, count is the max number of favorites */
  songfav->count = ilistGetCount (songfav->songfavList);

  songfav->songfavLookup = slistAlloc ("songfav-lookup", LIST_UNORDERED, NULL);
  slistSetSize (songfav->songfavLookup, songfav->count);

  ilistStartIterator (songfav->songfavList, &iteridx);
  while ((key = ilistIterateKey (songfav->songfavList, &iteridx)) >= 0) {
    const char  *name;
    const char  *disp;
    const char  *color;
    int         usersel;
    char        tbuff [100];

    name = ilistGetStr (songfav->songfavList, key, SONGFAV_NAME);
    disp = ilistGetStr (songfav->songfavList, key, SONGFAV_DISPLAY);
    color = ilistGetStr (songfav->songfavList, key, SONGFAV_COLOR);
    usersel = ilistGetNum (songfav->songfavList, key, SONGFAV_USERSEL);
    /* this is gtk specific stuff; it may need to be re-coded */
    /* if another gui toolkit is implemented */
    if (color == NULL || *color == '\0') {
      /* for key 0, simply use the display string by itself. */
      /* it will inherit the standard text color. */
      snprintf (tbuff, sizeof (tbuff), "%s", disp);
    } else {
      /* otherwise use html span to set the color. */
      /* this allows the color to show when selected. */
      snprintf (tbuff, sizeof (tbuff), "<span color=\"%s\">%s</span>",
          color, disp);
    }
    slistSetNum (songfav->songfavLookup, name, key);
    if (! usersel) {
      --songfav->count;
    }
  }

  slistSort (songfav->songfavLookup);

  return songfav;
}

void
songFavoriteFree (songfav_t *songfav)
{
  if (songfav != NULL) {
    datafileFree (songfav->df);
    slistFree (songfav->songfavLookup);
    mdfree (songfav);
  }
}

int
songFavoriteGetCount (songfav_t *songfav)
{
  if (songfav == NULL) {
    return 0;
  }

  return songfav->count;
}

int
songFavoriteGetNextValue (songfav_t *songfav, int value)
{
  if (value < 0) {
    value = 0;
  } else {
    /* the assumption here is that any non user selectable favorite */
    /* selections are placed at the end of the favorites.txt datafile */
    ++value;
    if (value >= songfav->count) {
      value = 0;
    }
  }

  return value;
}

const char *
songFavoriteGetStr (songfav_t *songfav, ilistidx_t key, int idx)
{
  if (songfav == NULL) {
    return "";
  }

  return ilistGetStr (songfav->songfavList, key, idx);
}

void
songFavoriteConv (datafileconv_t *conv)
{
  songfav_t   *songfav;
  ssize_t     num;

  songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
  if (songfav == NULL) {
    return;
  }

  if (conv->invt == VALUE_STR) {
    num = slistGetNum (songfav->songfavLookup, conv->str);
    if (num == LIST_VALUE_INVALID) {
      num = 0;
    }
    conv->outvt = VALUE_NUM;
    conv->num = num;
  } else if (conv->invt == VALUE_NUM) {
    num = conv->num;
    conv->outvt = VALUE_STR;
    conv->str = ilistGetStr (songfav->songfavList, num, SONGFAV_NAME);
  }
}

