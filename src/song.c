#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "dance.h"
#include "datafile.h"
#include "fileop.h"
#include "genre.h"
#include "level.h"
#include "nlist.h"
#include "log.h"
#include "rating.h"
#include "song.h"
#include "status.h"
#include "tagdef.h"

static void songConvFavorite (char *keydata, datafileret_t *ret);

  /* must be sorted in ascii order */
static datafilekey_t songdfkeys[] = {
  { "ADJUSTFLAGS",          TAG_ADJUSTFLAGS,          VALUE_DATA, NULL, -1 },
  { "AFMODTIME",            TAG_AFMODTIME,            VALUE_NUM, NULL, -1 },
  { "ALBUM",                TAG_ALBUM,                VALUE_DATA, NULL, -1 },
  { "ALBUMARTIST",          TAG_ALBUMARTIST,          VALUE_DATA, NULL, -1 },
  { "ARTIST",               TAG_ARTIST,               VALUE_DATA, NULL, -1 },
  { "AUTOORGFLAG",          TAG_AUTOORGFLAG,          VALUE_NUM, NULL, -1 },
  { "BPM",                  TAG_BPM,                  VALUE_NUM, NULL, -1 },
  { "COMPOSER",             TAG_COMPOSER,             VALUE_DATA, NULL, -1 },
  { "CONDUCTOR",            TAG_CONDUCTOR,            VALUE_DATA, NULL, -1 },
  { "DANCE",                TAG_DANCE,                VALUE_NUM, danceConvDance, -1 },
  { "DANCELEVEL",           TAG_DANCELEVEL,           VALUE_DATA, levelConv, -1 },
  { "DANCERATING",          TAG_DANCERATING,          VALUE_DATA, ratingConv, -1 },
  { "DATE",                 TAG_DATE,                 VALUE_DATA, NULL, -1 },
  { "DBADDDATE",            TAG_DBADDDATE,            VALUE_DATA, NULL, -1 },
  { "DISCNUMBER",           TAG_DISCNUMBER,           VALUE_NUM, NULL, -1 },
  { "DISCTOTAL",            TAG_DISCTOTAL,            VALUE_NUM, NULL, -1 },
  { "DISPLAYIMG",           TAG_DISPLAYIMG,           VALUE_DATA, NULL, -1 },
  { "DURATION",             TAG_DURATION,             VALUE_NUM, NULL, -1 },
  { "FAVORITE",             TAG_FAVORITE,             VALUE_NUM, songConvFavorite, -1 },
  { "FILE",                 TAG_FILE,                 VALUE_DATA, NULL, -1 },
  { "GENRE",                TAG_GENRE,                VALUE_NUM, genreConv, -1 },
  { "KEYWORD",              TAG_KEYWORD,              VALUE_DATA, NULL, -1 },
  { "MQDISPLAY",            TAG_MQDISPLAY,            VALUE_DATA, NULL, -1 },
  { "MUSICBRAINZ_TRACKID",  TAG_MUSICBRAINZ_TRACKID,  VALUE_DATA, NULL, -1 },
  { "NOMAXPLAYTIME",        TAG_NOMAXPLAYTIME,        VALUE_NUM, parseConvBoolean, -1 },
  { "NOTES",                TAG_NOTES,                VALUE_DATA, NULL, -1 },
  { "RRN",                  TAG_RRN,                  VALUE_NUM, NULL, -1 },
  { "SAMESONG",             TAG_SAMESONG,             VALUE_DATA, NULL, -1 },
  { "SONGEND",              TAG_SONGEND,              VALUE_NUM, NULL, -1 },
  { "SONGSTART",            TAG_SONGSTART,            VALUE_NUM, NULL, -1 },
  { "SPEEDADJUSTMENT",      TAG_SPEEDADJUSTMENT,      VALUE_NUM, NULL, -1 },
  { "STATUS",               TAG_STATUS,               VALUE_NUM, statusConv, -1 },
  { "TAGS",                 TAG_TAGS,                 VALUE_LIST, parseConvTextList, -1 },
  { "TITLE",                TAG_TITLE,                VALUE_DATA, NULL, -1 },
  { "TRACKNUMBER",          TAG_TRACKNUMBER,          VALUE_NUM, NULL, -1 },
  { "TRACKTOTAL",           TAG_TRACKTOTAL,           VALUE_NUM, NULL, -1 },
  { "UPDATEFLAG",           TAG_UPDATEFLAG,           VALUE_NUM, NULL, -1 },
  { "UPDATETIME",           TAG_UPDATETIME,           VALUE_NUM, NULL, -1 },
  { "VARIOUSARTISTS",       TAG_VARIOUSARTISTS,       VALUE_NUM, NULL, -1 },
  { "VOLUMEADJUSTPERC",     TAG_VOLUMEADJUSTPERC,     VALUE_DOUBLE, NULL, -1 },
  { "WRITETIME",            TAG_WRITETIME,            VALUE_NUM, NULL, -1 },
};
#define SONG_DFKEY_COUNT (sizeof (songdfkeys) / sizeof (datafilekey_t))

static datafilekey_t favoritedfkeys [SONG_FAVORITE_MAX] = {
  { "bluestar",   SONG_FAVORITE_BLUE,     VALUE_DATA, NULL, -1 },
  { "greenstar",  SONG_FAVORITE_GREEN,    VALUE_DATA, NULL, -1 },
  { "none",       SONG_FAVORITE_NONE,     VALUE_DATA, NULL, -1 },
  { "orangestar", SONG_FAVORITE_ORANGE,   VALUE_DATA, NULL, -1 },
  { "purplestar", SONG_FAVORITE_PURPLE,   VALUE_DATA, NULL, -1 },
  { "redstar",    SONG_FAVORITE_RED,      VALUE_DATA, NULL, -1 },
};

static songfavoriteinfo_t songfavoriteinfo [SONG_FAVORITE_MAX] = {
  { SONG_FAVORITE_NONE, "\xE2\x98\x86", "" },
  { SONG_FAVORITE_RED, "\xE2\x98\x85", "#9b3128" },
  { SONG_FAVORITE_ORANGE, "\xE2\x98\x85", "#c26a1a" },
  { SONG_FAVORITE_GREEN, "\xE2\x98\x85", "#00b25d", },
  { SONG_FAVORITE_BLUE, "\xE2\x98\x85", "#2f2ad7" },
  { SONG_FAVORITE_PURPLE, "\xE2\x98\x85", "#901ba3" },
};

song_t *
songAlloc (void)
{
  song_t    *song;

  song = malloc (sizeof (song_t));
  assert (song != NULL);
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
  }
}

void
songParse (song_t *song, char *data)
{
  song->songInfo = datafileParse (data, "song", DFTYPE_KEY_VAL,
      songdfkeys, SONG_DFKEY_COUNT, DATAFILE_NO_LOOKUP, NULL);
  nlistSort (song->songInfo);
}

char *
songGetData (song_t *song, nlistidx_t idx)
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

songfavoriteinfo_t *
songGetFavoriteData (song_t *song)
{
  ssize_t       value;

  if (song == NULL || song->songInfo == NULL) {
    return &songfavoriteinfo [SONG_FAVORITE_NONE];
  }

  value = nlistGetNum (song->songInfo, TAG_FAVORITE);
  if (value == LIST_VALUE_INVALID) {
    value = SONG_FAVORITE_NONE;
  }
  return songGetFavorite (value);
}

songfavoriteinfo_t *
songGetFavorite (int value)
{
  return &songfavoriteinfo [value];
}

void
songSetNum (song_t *song, nlistidx_t tagidx, ssize_t value)
{
  if (song == NULL || song->songInfo == NULL) {
    return;
  }

  nlistSetNum (song->songInfo, tagidx, value);
}

void
songSetData (song_t *song, nlistidx_t tagidx, char *str)
{
  if (song == NULL || song->songInfo == NULL) {
    return;
  }

  nlistSetData (song->songInfo, tagidx, str);
}

void
songChangeFavorite (song_t *song)
{
  ssize_t fav = SONG_FAVORITE_NONE;

  if (song == NULL || song->songInfo == NULL) {
    return;
  }

  fav = nlistGetNum (song->songInfo, TAG_FAVORITE);
  if (fav == LIST_VALUE_INVALID) {
    fav = SONG_FAVORITE_NONE;
  }
  ++fav;
  if (fav >= SONG_FAVORITE_MAX) {
    fav = SONG_FAVORITE_NONE;
  }
  nlistSetNum (song->songInfo, TAG_FAVORITE, fav);
}

bool
songAudioFileExists (song_t *song)
{
  char      tbuff [MAXPATHLEN];
  char      *sfname;

  sfname = songGetData (song, TAG_FILE);
  if (sfname [0] == '/' || (sfname [1] == ':' && sfname [2] == '/')) {
    strlcpy (tbuff, sfname, MAXPATHLEN);
  } else {
    snprintf (tbuff, MAXPATHLEN, "%s/%s",
        (char *) bdjoptGetData (OPT_M_DIR_MUSIC), sfname);
  }
  return fileopExists (tbuff);
}

/* internal routines */

static void
songConvFavorite (char *keydata, datafileret_t *ret)
{
  nlistidx_t       idx;

  ret->valuetype = VALUE_NUM;
  if (keydata == NULL || strcmp (keydata, "") == 0) {
    ret->u.num = SONG_FAVORITE_NONE;
    return;
  }
  idx = dfkeyBinarySearch (favoritedfkeys, SONG_FAVORITE_MAX, keydata);
  if (idx < 0) {
    ret->u.num = SONG_FAVORITE_NONE;
  }
  ret->u.num = favoritedfkeys [idx].itemkey;
}

