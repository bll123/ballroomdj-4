#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "bdjstring.h"
#include "dance.h"
#include "datafile.h"
#include "fileop.h"
#include "genre.h"
#include "ilist.h"
#include "level.h"
#include "nlist.h"
#include "log.h"
#include "rating.h"
#include "song.h"
#include "songfav.h"
#include "songutil.h"
#include "slist.h"
#include "status.h"
#include "tagdef.h"

#include "orgutil.h"

typedef struct song {
  nlist_t     *songInfo;
  bool        changed;
  bool        songlistchange;
} song_t;

static void songInit (void);
static void songCleanup (void);

/* must be sorted in ascii order */
static datafilekey_t songdfkeys [] = {
  { "ADJUSTFLAGS",          TAG_ADJUSTFLAGS,          VALUE_NUM, songConvAdjustFlags, -1 },
  { "ALBUM",                TAG_ALBUM,                VALUE_STR, NULL, -1 },
  { "ALBUMARTIST",          TAG_ALBUMARTIST,          VALUE_STR, NULL, -1 },
  { "ARTIST",               TAG_ARTIST,               VALUE_STR, NULL, -1 },
  { "BPM",                  TAG_BPM,                  VALUE_NUM, NULL, -1 },
  { "COMPOSER",             TAG_COMPOSER,             VALUE_STR, NULL, -1 },
  { "CONDUCTOR",            TAG_CONDUCTOR,            VALUE_STR, NULL, -1 },
  { "DANCE",                TAG_DANCE,                VALUE_NUM, danceConvDance, -1 },
  { "DANCELEVEL",           TAG_DANCELEVEL,           VALUE_NUM, levelConv, -1 },
  { "DANCERATING",          TAG_DANCERATING,          VALUE_NUM, ratingConv, -1 },
  { "DATE",                 TAG_DATE,                 VALUE_STR, NULL, -1 },
  { "DBADDDATE",            TAG_DBADDDATE,            VALUE_STR, NULL, -1 },
  { "DISC",                 TAG_DISCNUMBER,           VALUE_NUM, NULL, -1 },
  { "DISCTOTAL",            TAG_DISCTOTAL,            VALUE_NUM, NULL, -1 },
  { "DURATION",             TAG_DURATION,             VALUE_NUM, NULL, -1 },
  { "FAVORITE",             TAG_FAVORITE,             VALUE_NUM, songFavoriteConv, -1 },
  { "FILE",                 TAG_FILE,                 VALUE_STR, NULL, -1 },
  { "GENRE",                TAG_GENRE,                VALUE_NUM, genreConv, -1 },
  { "KEYWORD",              TAG_KEYWORD,              VALUE_STR, NULL, -1 },
  { "LASTUPDATED",          TAG_LAST_UPDATED,         VALUE_NUM, NULL, -1 },
  { "MQDISPLAY",            TAG_MQDISPLAY,            VALUE_STR, NULL, -1 },
  { "NOTES",                TAG_NOTES,                VALUE_STR, NULL, -1 },
  { "RECORDING_ID",         TAG_RECORDING_ID,         VALUE_STR, NULL, -1 },
  { "RRN",                  TAG_RRN,                  VALUE_NUM, NULL, -1 },
  { "SAMESONG",             TAG_SAMESONG,             VALUE_NUM, NULL, -1 },
  { "SONGEND",              TAG_SONGEND,              VALUE_NUM, NULL, -1 },
  { "SONGSTART",            TAG_SONGSTART,            VALUE_NUM, NULL, -1 },
  { "SPEEDADJUSTMENT",      TAG_SPEEDADJUSTMENT,      VALUE_NUM, NULL, -1 },
  { "STATUS",               TAG_STATUS,               VALUE_NUM, statusConv, -1 },
  { "TAGS",                 TAG_TAGS,                 VALUE_LIST, convTextList, -1 },
  { "TITLE",                TAG_TITLE,                VALUE_STR, NULL, -1 },
  { "TRACKNUMBER",          TAG_TRACKNUMBER,          VALUE_NUM, NULL, -1 },
  { "TRACKTOTAL",           TAG_TRACKTOTAL,           VALUE_NUM, NULL, -1 },
  { "TRACK_ID",             TAG_TRACK_ID,             VALUE_STR, NULL, -1 },
  { "VOLUMEADJUSTPERC",     TAG_VOLUMEADJUSTPERC,     VALUE_DOUBLE, NULL, -1 },
  { "WORK_ID",              TAG_WORK_ID,              VALUE_STR, NULL, -1 },
};
enum {
  SONG_DFKEY_COUNT = (sizeof (songdfkeys) / sizeof (datafilekey_t))
};

typedef struct {
  bool      initialized;
  long      songcount;
  level_t   *levels;
  songfav_t *songfav;
} songinit_t;

static songinit_t gsonginit = { false, 0, NULL, NULL };

song_t *
songAlloc (void)
{
  song_t    *song;

  songInit ();

  song = malloc (sizeof (song_t));
  assert (song != NULL);
  song->changed = false;
  song->songlistchange = false;
  song->songInfo = nlistAlloc ("song", LIST_ORDERED, free);
  ++gsonginit.songcount;
  return song;
}

void
songFree (void *tsong)
{
  song_t  *song = (song_t *) tsong;

  if (song != NULL) {
    if (song->songInfo != NULL) {
      nlistFree (song->songInfo);
    }
    free (song);
    --gsonginit.songcount;
    if (gsonginit.songcount <= 0) {
      songCleanup ();
    }
  }
}

void
songParse (song_t *song, char *data, ssize_t didx)
{
  char        tbuff [40];
  ilistidx_t  lkey;
  ssize_t     tval;

  if (song == NULL || data == NULL) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "song-%zd", didx);
  if (song->songInfo != NULL) {
    nlistFree (song->songInfo);
  }
  song->songInfo = datafileParse (data, tbuff, DFTYPE_KEY_VAL,
      songdfkeys, SONG_DFKEY_COUNT);
  nlistSort (song->songInfo);

  /* check and set some defaults */

  /* always set the temporary flag; if it needs to be true, the caller */
  /* will set it */
  nlistSetNum (song->songInfo, TAG_TEMPORARY, false);

  tval = nlistGetNum (song->songInfo, TAG_ADJUSTFLAGS);
  if (tval != LIST_VALUE_INVALID && (tval & SONG_ADJUST_INVALID)) {
    nlistSetNum (song->songInfo, TAG_ADJUSTFLAGS, LIST_VALUE_INVALID);
  }

  lkey = nlistGetNum (song->songInfo, TAG_DANCELEVEL);
  if (lkey < 0) {
    lkey = levelGetDefaultKey (gsonginit.levels);
    // Use default setting
    nlistSetNum (song->songInfo, TAG_DANCELEVEL, lkey);
  }

  lkey = nlistGetNum (song->songInfo, TAG_STATUS);
  if (lkey < 0) {
    // New
    nlistSetNum (song->songInfo, TAG_STATUS, 0);
  }

  lkey = nlistGetNum (song->songInfo, TAG_DANCERATING);
  if (lkey < 0) {
    // Unrated
    nlistSetNum (song->songInfo, TAG_DANCERATING, 0);
  }

  song->changed = false;
  song->songlistchange = false;
}

char *
songGetStr (song_t *song, nlistidx_t idx)
{
  char    *value;

  if (song == NULL || song->songInfo == NULL) {
    return NULL;
  }

  value = nlistGetStr (song->songInfo, idx);
  return value;
}

ssize_t
songGetNum (song_t *song, nlistidx_t idx)
{
  ssize_t     value;

  if (song == NULL || song->songInfo == NULL) {
    return LIST_VALUE_INVALID;
  }

  value = nlistGetNum (song->songInfo, idx);
  return value;
}

double
songGetDouble (song_t *song, nlistidx_t idx)
{
  double      value;

  if (song == NULL || song->songInfo == NULL) {
    return LIST_DOUBLE_INVALID;
  }

  value = nlistGetDouble (song->songInfo, idx);
  return value;
}

slist_t *
songGetList (song_t *song, nlistidx_t idx)
{
  slist_t   *value;

  if (song == NULL || song->songInfo == NULL) {
    return NULL;
  }

  value = nlistGetList (song->songInfo, idx);
  return value;
}

void
songSetNum (song_t *song, nlistidx_t tagidx, ssize_t value)
{
  if (song == NULL || song->songInfo == NULL) {
    return;
  }

  nlistSetNum (song->songInfo, tagidx, value);
  song->changed = true;
  if (tagidx == TAG_DANCE) {
    song->songlistchange = true;
  }
}

void
songSetDouble (song_t *song, nlistidx_t tagidx, double value)
{
  if (song == NULL || song->songInfo == NULL) {
    return;
  }

  nlistSetDouble (song->songInfo, tagidx, value);
  song->changed = true;
}

void
songSetStr (song_t *song, nlistidx_t tagidx, const char *str)
{
  if (song == NULL || song->songInfo == NULL) {
    return;
  }

  nlistSetStr (song->songInfo, tagidx, str);
  song->changed = true;
  if (tagidx == TAG_TITLE || tagidx == TAG_FILE) {
    song->songlistchange = true;
  }
}

void
songSetList (song_t *song, nlistidx_t tagidx, const char *str)
{
  dfConvFunc_t    convfunc;
  datafileconv_t  conv;
  slist_t         *slist = NULL;

  if (song == NULL || song->songInfo == NULL) {
    return;
  }

  conv.allocated = false;
  conv.str = (char *) str;
  conv.valuetype = VALUE_STR;
  convfunc = tagdefs [tagidx].convfunc;
  if (convfunc != NULL) {
    convfunc (&conv);
  }
  if (conv.valuetype != VALUE_LIST) {
    return;
  }

  slist = conv.list;
  nlistSetList (song->songInfo, tagidx, slist);
  song->changed = true;
}

void
songChangeFavorite (song_t *song)
{
  ssize_t fav = SONG_FAVORITE_NONE;

  if (song == NULL || song->songInfo == NULL) {
    return;
  }

  fav = nlistGetNum (song->songInfo, TAG_FAVORITE);
  fav = songFavoriteGetNextValue (gsonginit.songfav, fav);
  nlistSetNum (song->songInfo, TAG_FAVORITE, fav);
  song->changed = true;
}

bool
songAudioFileExists (song_t *song)
{
  char      *sfname;
  char      *ffn;
  bool      exists = false;

  sfname = songGetStr (song, TAG_FILE);
  ffn = songFullFileName (sfname);
  exists = false;
  if (ffn != NULL) {
    exists = fileopFileExists (ffn);
    free (ffn);
  }
  return exists;
}

/* used by the song editor via uisong to get the values for display */
/* used for tags that have a conversion set and also for strings */
/* favorite returns the span-display string used by gtk */
/*     <span color="...">X</span> */
char *
songDisplayString (song_t *song, int tagidx)
{
  valuetype_t     vt;
  dfConvFunc_t    convfunc;
  datafileconv_t  conv;
  char            *str = NULL;

  if (song == NULL) {
    return NULL;
  }

  if (tagidx == TAG_FAVORITE) {
    ilistidx_t  favidx;
    const char  *tstr;

    favidx = songGetNum (song, tagidx);
    tstr = songFavoriteGetSpanStr (gsonginit.songfav, favidx);
    if (tstr != NULL) {
      str = strdup (tstr);
    }
    return str;
  }

  vt = tagdefs [tagidx].valueType;

  convfunc = tagdefs [tagidx].convfunc;
  if (convfunc != NULL) {
    conv.allocated = false;
    if (vt == VALUE_NUM) {
      conv.num = songGetNum (song, tagidx);
    } else if (vt == VALUE_LIST) {
      conv.list = songGetList (song, tagidx);
    }
    conv.valuetype = vt;
    convfunc (&conv);
    if (conv.str == NULL) { conv.str = ""; }
    str = strdup (conv.str);
    if (conv.allocated) {
      free (conv.str);
    }
  } else {
    str = songGetStr (song, tagidx);
    if (str == NULL) { str = ""; }
    str = strdup (str);
  }

  return str;
}

slist_t *
songTagList (song_t *song)
{
  slist_t   *taglist;

  taglist = datafileSaveKeyValList ("song-tag", songdfkeys, SONG_DFKEY_COUNT, song->songInfo);
  return taglist;
}

inline bool
songIsChanged (song_t *song)
{
  return song->changed;
}

inline bool
songHasSonglistChange (song_t *song)
{
  return song->songlistchange;
}

inline void
songClearChanged (song_t *song)
{
  song->changed = false;
  song->songlistchange = false;
}

/* internal routines */

static void
songInit (void)
{
  if (gsonginit.initialized) {
    return;
  }
  gsonginit.initialized = true;
  gsonginit.songcount = 0;

  gsonginit.levels = bdjvarsdfGet (BDJVDF_LEVELS);
  gsonginit.songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
}

static void
songCleanup (void)
{
  if (! gsonginit.initialized) {
    return;
  }

  gsonginit.songcount = 0;
  gsonginit.initialized = false;
}


