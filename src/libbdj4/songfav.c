#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "nlist.h"
#include "pathbld.h"
#include "slist.h"
#include "songfav.h"

/* must be sorted in ascii order */
static datafilekey_t songfavdfkeys [SONGFAV_KEY_MAX] = {
  { "COLOR",    SONGFAV_COLOR,    VALUE_STR, NULL, -1 },
  { "DISPLAY",  SONGFAV_DISPLAY,  VALUE_STR, NULL, -1 },
  { "NAME",     SONGFAV_NAME,     VALUE_STR, NULL, -1 },
};

typedef struct songfav {
  datafile_t    *df;
  ilist_t       *songfavList;
  slist_t       *songfavLookup;
  nlist_t       *spanstrList;
  int           count;
} songfav_t;

// static bool initialized = false;

songfav_t *
songFavoriteAlloc (void)
{
  char        fname [MAXPATHLEN];
  songfav_t   *songfav;
  ilistidx_t  iteridx;
  ilistidx_t  key;

  pathbldMakePath (fname, sizeof (fname), "favorites",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: favorites: missing %s", fname);
    return NULL;
  }

  songfav = malloc (sizeof (songfav_t));

  songfav->df = datafileAllocParse ("favorites", DFTYPE_INDIRECT, fname,
        songfavdfkeys, SONGFAV_KEY_MAX);
  songfav->songfavList = datafileGetList (songfav->df);
  songfav->count = ilistGetCount (songfav->songfavList);

  songfav->spanstrList = nlistAlloc ("songfav-span", LIST_UNORDERED, free);
  nlistSetSize (songfav->spanstrList, songfav->count);
  songfav->songfavLookup = slistAlloc ("songfav-lookup", LIST_UNORDERED, NULL);
  slistSetSize (songfav->songfavLookup, songfav->count);

  ilistStartIterator (songfav->songfavList, &iteridx);
  while ((key = ilistIterateKey (songfav->songfavList, &iteridx)) >= 0) {
    char    *name;
    char    *disp;
    char    *color;
    char  tbuff [100];

    name = ilistGetStr (songfav->songfavList, key, SONGFAV_NAME);
    disp = ilistGetStr (songfav->songfavList, key, SONGFAV_DISPLAY);
    color = ilistGetStr (songfav->songfavList, key, SONGFAV_COLOR);
    if (color == NULL) {
      color = "#ffffff";
    }
    snprintf (tbuff, sizeof (tbuff), "<span color=\"%s\">%s</span>",
          color, disp);
    nlistSetStr (songfav->spanstrList, key, tbuff);
    slistSetNum (songfav->songfavLookup, name, key);
  }

  nlistSort (songfav->spanstrList);
  slistSort (songfav->songfavLookup);

  return songfav;
}

void
songFavoriteFree (songfav_t *songfav)
{
  if (songfav != NULL) {
    datafileFree (songfav->df);
    nlistFree (songfav->spanstrList);
    free (songfav);
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
    ++value;
  }
  if (value >= songfav->count) {
    value = 0;
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

const char *
songFavoriteGetSpanStr (songfav_t *songfav, ilistidx_t key)
{
  if (songfav == NULL) {
    return "";
  }

  return nlistGetStr (songfav->spanstrList, key);
}

void
songFavoriteConv (datafileconv_t *conv)
{
  songfav_t   *songfav;
  ssize_t     num;

  songfav = bdjvarsdfGet (BDJVDF_FAVORITES);

  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    conv->valuetype = VALUE_NUM;
    num = slistGetNum (songfav->songfavLookup, conv->str);
    if (num == LIST_VALUE_INVALID) {
      num = 0;
    }
    conv->num = num;
  } else if (conv->valuetype == VALUE_NUM) {
    conv->valuetype = VALUE_STR;
    num = conv->num;
    conv->str = ilistGetStr (songfav->songfavList, num, SONGFAV_NAME);
  }
}

