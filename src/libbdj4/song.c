/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <string.h>
#include <inttypes.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "datafile.h"
#include "fileop.h"
#include "genre.h"
#include "ilist.h"
#include "level.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"    /* need db flags */
#include "nlist.h"
#include "rating.h"
#include "slist.h"
#include "songfav.h"
#include "song.h"
#include "songutil.h"
#include "status.h"
#include "tagdef.h"
#include "tmutil.h"

#include "orgutil.h"

typedef struct song {
  uint64_t    ident;
  nlist_t     *songInfo;
  bool        changed;
  bool        songlistchange;
} song_t;

enum {
  TEMP_TAG_DBADD = TAG_KEY_MAX,
  SONG_IDENT = 0xccbbaa00676e6f73,
};

static void songInit (void);
static void songCleanup (void);

/* must be sorted in ascii order */
static datafilekey_t songdfkeys [] = {
  { "ADJUSTFLAGS",          TAG_ADJUSTFLAGS,          VALUE_NUM, songutilConvAdjustFlags, DF_NORM },
  { "ALBUM",                TAG_ALBUM,                VALUE_STR, NULL, DF_NORM },
  { "ALBUMARTIST",          TAG_ALBUMARTIST,          VALUE_STR, NULL, DF_NORM },
  { "ALBUMARTISTSORT",      TAG_SORT_ALBUMARTIST,     VALUE_STR, NULL, DF_NORM },
  { "ALBUMSORT",            TAG_SORT_ALBUM,           VALUE_STR, NULL, DF_NORM },
  { "ARTIST",               TAG_ARTIST,               VALUE_STR, NULL, DF_NORM },
  { "ARTISTSORT",           TAG_SORT_ARTIST,          VALUE_STR, NULL, DF_NORM },
  { "BPM",                  TAG_BPM,                  VALUE_NUM, NULL, DF_NORM },
  { "COMMENT",              TAG_COMMENT,              VALUE_STR, NULL, DF_NORM },
  { "COMPOSER",             TAG_COMPOSER,             VALUE_STR, NULL, DF_NORM },
  { "COMPOSERSORT",         TAG_SORT_COMPOSER,        VALUE_STR, NULL, DF_NORM },
  { "CONDUCTOR",            TAG_CONDUCTOR,            VALUE_STR, NULL, DF_NORM },
  { "DANCE",                TAG_DANCE,                VALUE_NUM, danceConvDance, DF_NORM },
  { "DANCELEVEL",           TAG_DANCELEVEL,           VALUE_NUM, levelConv, DF_NORM },
  { "DANCERATING",          TAG_DANCERATING,          VALUE_NUM, ratingConv, DF_NORM },
  { "DATE",                 TAG_DATE,                 VALUE_STR, NULL, DF_NORM },
  /* the old db-add-date was removed in 4.8.3 */
  /* as these are different types, the old version must be converted */
  { "DBADDDATE",            TEMP_TAG_DBADD,           VALUE_STR, NULL, DF_NO_WRITE },
  { "DB_ADD_DATE",          TAG_DBADDDATE,            VALUE_NUM, NULL, DF_NORM },
  { "DB_LOC_LOCK",          TAG_DB_LOC_LOCK,          VALUE_NUM, convBoolean, DF_NORM },
  { "DISC",                 TAG_DISCNUMBER,           VALUE_NUM, NULL, DF_NORM },
  { "DISCTOTAL",            TAG_DISCTOTAL,            VALUE_NUM, NULL, DF_NORM },
  /* no conversion is defined for duration. it is handled as */
  /* a special case in uisong.c */
  { "DURATION",             TAG_DURATION,             VALUE_NUM, NULL, DF_NORM },
  { "FAVORITE",             TAG_FAVORITE,             VALUE_NUM, songFavoriteConv, DF_NORM },
  { "FILE",                 TAG_URI,                  VALUE_STR, NULL, DF_NO_WRITE },
  { "GENRE",                TAG_GENRE,                VALUE_NUM, genreConv, DF_NORM },
  { "GROUPING",             TAG_GROUPING,             VALUE_STR, NULL, DF_NORM },
  { "KEYWORD",              TAG_KEYWORD,              VALUE_STR, NULL, DF_NORM },
  { "LASTUPDATED",          TAG_LAST_UPDATED,         VALUE_NUM, NULL, DF_NORM },
  { "MOVEMENTCOUNT",        TAG_MOVEMENTCOUNT,        VALUE_NUM, NULL, DF_NORM },
  { "MOVEMENTNAME",         TAG_MOVEMENTNAME,         VALUE_STR, NULL, DF_NORM },
  { "MOVEMENTNUM",          TAG_MOVEMENTNUM,          VALUE_NUM, NULL, DF_NORM },
  { "MQDISPLAY",            TAG_MQDISPLAY,            VALUE_STR, NULL, DF_NORM },
  { "NOPLAYTMLIMIT",        TAG_NO_PLAY_TM_LIMIT,       VALUE_NUM, convBoolean, DF_NORM },
  { "NOTES",                TAG_NOTES,                VALUE_STR, NULL, DF_NORM },
  { "PFXLEN",               TAG_PREFIX_LEN,           VALUE_NUM, NULL, DF_NORM },
  { "RECORDING_ID",         TAG_RECORDING_ID,         VALUE_STR, NULL, DF_NORM },
  { "SAMESONG",             TAG_SAMESONG,             VALUE_NUM, NULL, DF_NORM },
  { "SONGEND",              TAG_SONGEND,              VALUE_NUM, NULL, DF_NORM },
  { "SONGSTART",            TAG_SONGSTART,            VALUE_NUM, NULL, DF_NORM },
  { "SONGTYPE",             TAG_SONG_TYPE,            VALUE_NUM, songutilConvSongType, DF_NORM },
  { "SPEEDADJUSTMENT",      TAG_SPEEDADJUSTMENT,      VALUE_NUM, NULL, DF_NORM },
  { "STATUS",               TAG_STATUS,               VALUE_NUM, statusConv, DF_NORM },
  { "TAGS",                 TAG_TAGS,                 VALUE_LIST, convTextList, DF_NORM },
  { "TITLE",                TAG_TITLE,                VALUE_STR, NULL, DF_NORM },
  { "TITLESORT",            TAG_SORT_TITLE,           VALUE_STR, NULL, DF_NORM },
  { "TRACKNUMBER",          TAG_TRACKNUMBER,          VALUE_NUM, NULL, DF_NORM },
  { "TRACKTOTAL",           TAG_TRACKTOTAL,           VALUE_NUM, NULL, DF_NORM },
  { "TRACK_ID",             TAG_TRACK_ID,             VALUE_STR, NULL, DF_NORM },
  { "URI",                  TAG_URI,                  VALUE_STR, NULL, DF_NORM },
  { "VOLUMEADJUSTPERC",     TAG_VOLUMEADJUSTPERC,     VALUE_DOUBLE, NULL, DF_NORM },
  { "WORK",                 TAG_WORK,                 VALUE_STR, NULL, DF_NORM },
  { "WORK_ID",              TAG_WORK_ID,              VALUE_STR, NULL, DF_NORM },
};
enum {
  SONG_DFKEY_COUNT = (sizeof (songdfkeys) / sizeof (datafilekey_t))
};

typedef struct {
  volatile atomic_flag  initialized;
  level_t               *levels;
  songfav_t             *songfav;
  bdjregex_t            *alldigits;
  bdjregex_t            *titlesort;
} songinit_t;

static songinit_t gsonginit = { ATOMIC_FLAG_INIT, NULL, NULL, NULL, NULL };

static void songSetDefaults (song_t *song);

song_t *
songAlloc (void)
{
  song_t    *song;

  songInit ();

  song = mdmalloc (sizeof (song_t));
  song->ident = SONG_IDENT;
  song->changed = false;
  song->songlistchange = false;
  song->songInfo = nlistAlloc ("song", LIST_ORDERED, NULL);

  return song;
}

void
songFree (void *tsong)
{
  song_t  *song = (song_t *) tsong;

  if (song == NULL || song->ident != SONG_IDENT) {
    return;
  }

  nlistFree (song->songInfo);
  song->ident = BDJ4_IDENT_FREE;
  mdfree (song);
}

void
songFromTagList (song_t *song, slist_t *tagdata)
{
  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return;
  }

  nlistFree (song->songInfo);
  song->songInfo = nlistAlloc ("song", LIST_ORDERED, NULL);

  for (int i = 0; i < SONG_DFKEY_COUNT; ++i) {
    const char  *tstr;

    if (songdfkeys [i].writeFlag == DF_NO_WRITE) {
      continue;
    }

    tstr = slistGetStr (tagdata, songdfkeys [i].name);
    if (tstr == NULL || ! *tstr) {
      continue;
    }

    if (songdfkeys [i].convFunc != NULL) {
      datafileconv_t    conv;

      conv.invt = VALUE_STR;
      conv.outvt = songdfkeys [i].valuetype;
      conv.str = tstr;
      songdfkeys [i].convFunc (&conv);
      if (conv.outvt == VALUE_NUM) {
        songSetNum (song, songdfkeys [i].itemkey, conv.num);
      }
      if (conv.outvt == VALUE_LIST) {
        nlistSetList (song->songInfo, songdfkeys [i].itemkey, conv.list);
      }
      if (conv.outvt == VALUE_DOUBLE) {
        songSetDouble (song, songdfkeys [i].itemkey, conv.dval);
      }
    } else if (songdfkeys [i].valuetype == VALUE_NUM) {
      songSetNum (song, songdfkeys [i].itemkey, atoll (tstr));
    } else if (songdfkeys [i].valuetype == VALUE_DOUBLE) {
      songSetDouble (song, songdfkeys [i].itemkey, atof (tstr) / DF_DOUBLE_MULT);
    } else {
      if (songdfkeys [i].itemkey == TAG_GROUPING) {
        /* 2025-2-19 : Some companies set the grouping tag to an all numeric */
        /*    group.  If an all numeric group is found, clear it */
        if (regexMatch (gsonginit.alldigits, tstr)) {
          tstr = NULL;
        }
      }
      if (songdfkeys [i].itemkey == TAG_SORT_TITLE) {
        /* 2025-5-23 : Some companies set the title-sort tag to the genre */
        if (regexMatch (gsonginit.titlesort, tstr)) {
          tstr = NULL;
        }
      }
      songSetStr (song, songdfkeys [i].itemkey, tstr);
    }
  }

  songSetDefaults (song);

  song->changed = false;
  song->songlistchange = false;
}

void
songParse (song_t *song, char *data, ilistidx_t dbidx)
{
  char        tbuff [40];

  if (song == NULL || data == NULL || song->ident != SONG_IDENT) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "song-%" PRId32, dbidx);
  nlistFree (song->songInfo);
  song->songInfo = datafileParse (data, tbuff, DFTYPE_KEY_VAL,
      songdfkeys, SONG_DFKEY_COUNT, NULL);
  nlistSort (song->songInfo);

  songSetDefaults (song);

  song->changed = false;
  song->songlistchange = false;
}

const char *
songGetStr (const song_t *song, nlistidx_t idx)
{
  const char  *value;

  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return NULL;
  }

  value = nlistGetStr (song->songInfo, idx);
  return value;
}

listnum_t
songGetNum (const song_t *song, nlistidx_t idx)
{
  ssize_t     value;

  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return LIST_VALUE_INVALID;
  }

  value = nlistGetNum (song->songInfo, idx);
  if (idx == TAG_FAVORITE &&
      songGetNum (song, TAG_DB_FLAGS) == MUSICDB_REMOVE_MARK) {
    datafileconv_t  conv;

    /* if the song is marked as removed, force the favorite display */
    /* to be 'removed' */
    conv.invt = VALUE_STR;
    conv.str = "removed";
    songFavoriteConv (&conv);
    value = conv.num;
  }

  return value;
}

double
songGetDouble (const song_t *song, nlistidx_t idx)
{
  double      value;

  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return LIST_DOUBLE_INVALID;
  }

  value = nlistGetDouble (song->songInfo, idx);
  return value;
}

slist_t *
songGetList (const song_t *song, nlistidx_t idx)
{
  slist_t   *value;

  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return NULL;
  }

  value = nlistGetList (song->songInfo, idx);
  return value;
}

void
songSetNum (song_t *song, nlistidx_t tagidx, listnum_t value)
{
  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
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
  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return;
  }

  nlistSetDouble (song->songInfo, tagidx, value);
  song->changed = true;
}

void
songSetStr (song_t *song, nlistidx_t tagidx, const char *str)
{
  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return;
  }

  nlistSetStr (song->songInfo, tagidx, str);
  song->changed = true;
  if (tagidx == TAG_TITLE || tagidx == TAG_URI) {
    song->songlistchange = true;
  }
}

void
songSetList (song_t *song, nlistidx_t tagidx, const char *str)
{
  dfConvFunc_t    convfunc;
  datafileconv_t  conv;
  slist_t         *slist = NULL;

  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return;
  }

  conv.str = str;
  conv.invt = VALUE_STR;
  convfunc = tagdefs [tagidx].convfunc;
  if (convfunc != NULL) {
    convfunc (&conv);
  }
  if (conv.outvt != VALUE_LIST) {
    return;
  }

  slist = conv.list;
  nlistSetList (song->songInfo, tagidx, slist);
  song->changed = true;
}

void
songChangeFavorite (song_t *song)
{
  int fav = SONG_FAVORITE_NONE;

  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return;
  }

  fav = nlistGetNum (song->songInfo, TAG_FAVORITE);
  if (fav < 0) {
    fav = SONG_FAVORITE_NONE;
  }
  fav = songFavoriteGetNextValue (gsonginit.songfav, fav);
  nlistSetNum (song->songInfo, TAG_FAVORITE, fav);
  song->changed = true;
}

bool
songAudioSourceExists (song_t *song)
{
  const char  *sfname;

  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return false;
  }

  sfname = songGetStr (song, TAG_URI);
  return audiosrcExists (sfname);
}

/* Used by the song editor via uisong to get the display strings. */
/* Used for tags that have a conversion set and also for strings. */
char *
songDisplayString (song_t *song, int tagidx, int flag)
{
  valuetype_t     vt;
  dfConvFunc_t    convfunc;
  datafileconv_t  conv;
  char            *str = NULL;
  const char      *tstr = NULL;

  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return NULL;
  }

  if (tagidx == TAG_FAVORITE) {
    ilistidx_t  favkey;
    const char  *tstr;

    favkey = songGetNum (song, tagidx);
    if (favkey < 0) {
      /* get the not-set display */
      favkey = 0;
    }
    tstr = songFavoriteGetStr (gsonginit.songfav, favkey, SONGFAV_DISPLAY);
    str = mdstrdup (tstr);
    return str;
  }

  if (tagidx == TAG_DBADDDATE) {
    time_t    val;
    char      buff [100];

    val = songGetNum (song, tagidx);
    /* pass in as milliseconds */
    tmutilToDateHM (val * 1000, buff, sizeof (buff));
    str = mdstrdup (buff);
    return str;
  }

  if (tagidx == TAG_DURATION) {
    long    dur;
    long    val;
    int     speed;
    char    durstr [50];
    char    tbuff [50];
    bool    changed = false;

    dur = songGetNum (song, TAG_DURATION);
    tmutilToMSD (dur, durstr, sizeof (durstr), 1);

    val = songGetNum (song, TAG_SONGEND);
    if (val > 0 && val < dur) {
      dur = val;
      changed = true;
    }
    val = songGetNum (song, TAG_SONGSTART);
    if (val > 0) {
      dur -= val;
      changed = true;
    }
    speed = songGetNum (song, TAG_SPEEDADJUSTMENT);
    if (speed > 0 && speed != 100) {
      dur = songutilAdjustPosReal (dur, speed);
      changed = true;
    }
    if (changed) {
      char  tmp [40];

      tmutilToMSD (dur, tmp, sizeof (tmp), 1);
      if (flag == SONG_UNADJUSTED_DURATION) {
        str = mdstrdup (durstr);
      } else if (flag == SONG_LONG_DURATION) {
        snprintf (tbuff, sizeof (tbuff), "%s (%s)", durstr, tmp);
        str = mdstrdup (tbuff);
      } else {
        str = mdstrdup (tmp);
      }
    } else {
      str = mdstrdup (durstr);
    }

    return str;
  }

  vt = tagdefs [tagidx].valueType;

  convfunc = tagdefs [tagidx].convfunc;

  if (convfunc != NULL) {
    if (vt == VALUE_NUM) {
      conv.num = songGetNum (song, tagidx);
    } else if (vt == VALUE_LIST) {
      conv.list = songGetList (song, tagidx);
    }
    conv.invt = vt;
    convfunc (&conv);
    if (conv.outvt == VALUE_STR) {
      if (conv.str == NULL) { conv.str = ""; }
      str = mdstrdup (conv.str);
    }
    if (conv.outvt == VALUE_STRVAL) {
      str = mdstrdup (conv.strval);
      dataFree (conv.strval);
    }

    /* if the marquee display is set, it should be listed */
    /* as an addition to the dance */
    if (tagidx == TAG_DANCE) {
      char  tbuff [200];

      tstr = songGetStr (song, TAG_MQDISPLAY);
      if (tstr != NULL && *tstr) {
        snprintf (tbuff, sizeof (tbuff), "%s (%s)", str, tstr);
        dataFree (str);
        str = mdstrdup (tbuff);
      }
    }
  } else {
    tstr = songGetStr (song, tagidx);
    if (tstr == NULL) { tstr = ""; }
    str = mdstrdup (tstr);
  }

  return str;
}

slist_t *
songTagList (song_t *song)
{
  slist_t   *taglist;

  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return NULL;
  }

  taglist = datafileSaveKeyValList ("song-tag", songdfkeys, SONG_DFKEY_COUNT, song->songInfo);
  return taglist;
}

char *
songCreateSaveData (song_t *song)
{
  char      *sbuffer;

  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return NULL;
  }

  sbuffer = mdmalloc (MUSICDB_MAX_SAVE);
  datafileSaveKeyValBuffer (sbuffer, MUSICDB_MAX_SAVE, "song-buff",
      songdfkeys, SONG_DFKEY_COUNT, song->songInfo, 0, DF_SKIP_EMPTY);
  return sbuffer;
}

bool
songIsChanged (song_t *song)
{
  if (song == NULL || song->ident != SONG_IDENT) {
    return false;
  }
  return song->changed;
}

void
songSetChanged (song_t *song)
{
  if (song == NULL || song->ident != SONG_IDENT) {
    return;
  }
  song->changed = true;
}

bool
songHasSonglistChange (song_t *song)
{
  if (song == NULL || song->ident != SONG_IDENT) {
    return false;
  }
  return song->songlistchange;
}

void
songClearChanged (song_t *song)
{
  if (song == NULL || song->ident != SONG_IDENT) {
    return;
  }

  song->changed = false;
  song->songlistchange = false;
}

void
songGetClassicalWork (const song_t *song, char *work, size_t sz)
{
  const char    *title;
  char          *p;

  if (song == NULL || song->ident != SONG_IDENT || song->songInfo == NULL) {
    return;
  }

  *work = '\0';
  title = nlistGetStr (song->songInfo, TAG_TITLE);
  p = strchr (title, ':');
  if (p != NULL) {
    stpecpy (work, work + sz, title);
    p = strchr (work, ':');
    *p = '\0';
  }
}


/* internal routines */

static void
songInit (void)
{
  if (atomic_flag_test_and_set (&gsonginit.initialized)) {
    return;
  }

  gsonginit.levels = bdjvarsdfGet (BDJVDF_LEVELS);
  gsonginit.songfav = bdjvarsdfGet (BDJVDF_FAVORITES);
  gsonginit.alldigits = regexInit ("^\\d+$");
  /* unfortunately, this is probably not all of the different genres */
  /* that are being put into the title-sort tag */
  gsonginit.titlesort = regexInit ("^(Orchestra|Pop|Soundtrack|"
      "World Music|Classic Music|Easy Listening|"
      "Latin Pop|Musette & Chanson/Folk/Gipsy|"
      "Nu Jazz/Electro Swing|Rhythm & Blues/Rockabilly|"
      "Soul/Gospel|Swing)$");
  atexit (songCleanup);
}

static void
songCleanup (void)
{
  if (! atomic_flag_test_and_set (&gsonginit.initialized)) {
    atomic_flag_clear (&gsonginit.initialized);
    return;
  }

  if (gsonginit.alldigits != NULL) {
    regexFree (gsonginit.alldigits);
    gsonginit.alldigits = NULL;
  }
  if (gsonginit.titlesort != NULL) {
    regexFree (gsonginit.titlesort);
    gsonginit.titlesort = NULL;
  }
  atomic_flag_clear (&gsonginit.initialized);
}

#if 0 /* for debugging */
void
songDump (song_t *song)   /* KEEP */
{
  if (song == NULL) {
    return;
  }

  for (int i = 0; i < SONG_DFKEY_COUNT; ++i) {
    switch (songdfkeys [i].valuetype) {
      case VALUE_STR: {
        fprintf (stderr, "%s : %s\n", songdfkeys [i].name, songGetStr (song, songdfkeys [i].itemkey));
        break;
      }
      case VALUE_NUM: {
        fprintf (stderr, "%s : %" PRId64 "\n", songdfkeys [i].name, songGetNum (song, songdfkeys [i].itemkey));
        break;
      }
      case VALUE_DOUBLE: {
        fprintf (stderr, "%s : %.2f\n", songdfkeys [i].name, songGetDouble (song, songdfkeys [i].itemkey));
        break;
      }
      default: {
        break;
      }
    }
  }
}
#endif

/* internal routines */

static void
songSetDefaults (song_t *song)
{
  ilistidx_t  lkey;
  ssize_t     tval;

  /* check and set some defaults */

  /* always set the db flags; if it needs to be set, */
  /* the caller will set it */
  nlistSetNum (song->songInfo, TAG_DB_FLAGS, MUSICDB_STD);

  tval = nlistGetNum (song->songInfo, TAG_ADJUSTFLAGS);
  if (tval < 0 ||
      (tval != LIST_VALUE_INVALID && (tval & SONG_ADJUST_INVALID))) {
    nlistSetNum (song->songInfo, TAG_ADJUSTFLAGS, SONG_ADJUST_NONE);
  }

  lkey = nlistGetNum (song->songInfo, TAG_DANCELEVEL);
  if (lkey < 0) {
    lkey = levelGetDefaultKey (gsonginit.levels);
    /* Use default setting */
    nlistSetNum (song->songInfo, TAG_DANCELEVEL, lkey);
  }

  lkey = nlistGetNum (song->songInfo, TAG_STATUS);
  if (lkey < 0) {
    /* New */
    nlistSetNum (song->songInfo, TAG_STATUS, 0);
  }

  lkey = nlistGetNum (song->songInfo, TAG_DANCERATING);
  if (lkey < 0) {
    /* Unrated */
    nlistSetNum (song->songInfo, TAG_DANCERATING, 0);
  }

  /* 2023-12-19: db location lock */
  lkey = nlistGetNum (song->songInfo, TAG_DB_LOC_LOCK);
  if (lkey < 0) {
    /* false */
    nlistSetNum (song->songInfo, TAG_DB_LOC_LOCK, 0);
  }

  /* 2024-4-17: 4.8.3 convert old db-add-date string to timestamp */
  {
    const char  *tstr;
    time_t      tmval;

    tstr = nlistGetStr (song->songInfo, TEMP_TAG_DBADD);
    tmval = nlistGetNum (song->songInfo, TAG_DBADDDATE);
    /* the song should never have both old and new set, but check */
    if (tstr != NULL && tmval < 0) {
      tmval = tmutilStringToUTC (tstr, "%F");
      nlistSetNum (song->songInfo, TAG_DBADDDATE, tmval);
    }
  }

  /* 2025-4-26: 4.15.0 no-max-play-time */
  lkey = nlistGetNum (song->songInfo, TAG_NO_PLAY_TM_LIMIT);
  if (lkey < 0) {
    /* false */
    nlistSetNum (song->songInfo, TAG_NO_PLAY_TM_LIMIT, 0);
  }
}
