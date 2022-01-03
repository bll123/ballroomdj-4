#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "song.h"
#include "list.h"
#include "bdjstring.h"
#include "tagdef.h"
#include "log.h"
#include "datafile.h"
#include "level.h"
#include "rating.h"

  /* must be sorted in ascii order */
static datafilekey_t songdfkeys[] = {
  { "ADJUSTFLAGS",          TAG_KEY_ADJUSTFLAGS,          VALUE_DATA, NULL },
  { "AFMODTIME",            TAG_KEY_AFMODTIME,            VALUE_LONG, NULL },
  { "ALBUM",                TAG_KEY_ALBUM,                VALUE_DATA, NULL },
  { "ALBUMARTIST",          TAG_KEY_ALBUMARTIST,          VALUE_DATA, NULL },
  { "ARTIST",               TAG_KEY_ARTIST,               VALUE_DATA, NULL },
  { "AUTOORGFLAG",          TAG_KEY_AUTOORGFLAG,          VALUE_LONG, NULL },
  { "BPM",                  TAG_KEY_BPM,                  VALUE_LONG, NULL },
  { "COMPOSER",             TAG_KEY_COMPOSER,             VALUE_DATA, NULL },
  { "CONDUCTOR",            TAG_KEY_CONDUCTOR,            VALUE_DATA, NULL },
  { "DANCE",                TAG_KEY_DANCE,                VALUE_DATA, NULL },
  { "DANCELEVEL",           TAG_KEY_DANCELEVEL,           VALUE_DATA, levelConv },
  { "DANCERATING",          TAG_KEY_DANCERATING,          VALUE_DATA, ratingConv },
  { "DATE",                 TAG_KEY_DATE,                 VALUE_DATA, NULL },
  { "DBADDDATE",            TAG_KEY_DBADDDATE,            VALUE_DATA, NULL },
  { "DISCNUMBER",           TAG_KEY_DISCNUMBER,           VALUE_LONG, NULL },
  { "DISCTOTAL",            TAG_KEY_DISCTOTAL,            VALUE_LONG, NULL },
  { "DISPLAYIMG",           TAG_KEY_DISPLAYIMG,           VALUE_DATA, NULL },
  { "DUR",                  TAG_KEY_DUR,                  VALUE_LONG, NULL },
  { "DURATION",             TAG_KEY_DURATION,             VALUE_DATA, NULL },
  { "DURATION_HMS",         TAG_KEY_DURATION_HMS,         VALUE_DOUBLE, NULL },
  { "DURATION_STR",         TAG_KEY_DURATION_STR,         VALUE_DATA, NULL },
  { "FILE",                 TAG_KEY_FILE,                 VALUE_DATA, NULL },
  { "GENRE",                TAG_KEY_GENRE,                VALUE_DATA, NULL },
  { "KEYWORD",              TAG_KEY_KEYWORD,              VALUE_DATA, NULL },
  { "MQDISPLAY",            TAG_KEY_MQDISPLAY,            VALUE_DATA, NULL },
  { "MUSICBRAINZ_TRACKID",  TAG_KEY_MUSICBRAINZ_TRACKID,  VALUE_DATA, NULL },
  { "NOMAXPLAYTIME",        TAG_KEY_NOMAXPLAYTIME,        VALUE_DATA, NULL },
  { "NOTES",                TAG_KEY_NOTES,                VALUE_DATA, NULL },
  { "RRN",                  TAG_KEY_RRN,                  VALUE_LONG, NULL },
  { "SAMESONG",             TAG_KEY_SAMESONG,             VALUE_DATA, NULL },
  { "SONGEND",              TAG_KEY_SONGEND,              VALUE_DATA, NULL },
  { "SONGSTART",            TAG_KEY_SONGSTART,            VALUE_DATA, NULL },
  { "SPEEDADJUSTMENT",      TAG_KEY_SPEEDADJUSTMENT,      VALUE_LONG, NULL },
  { "STATUS",               TAG_KEY_STATUS,               VALUE_DATA, NULL },
  { "TAGS",                 TAG_KEY_TAGS,                 VALUE_LIST, parseConvTextList },
  { "TITLE",                TAG_KEY_TITLE,                VALUE_DATA, NULL },
  { "TRACKNUMBER",          TAG_KEY_TRACKNUMBER,          VALUE_LONG, NULL },
  { "TRACKTOTAL",           TAG_KEY_TRACKTOTAL,           VALUE_LONG, NULL },
  { "UPDATEFLAG",           TAG_KEY_VARIOUSARTISTS,       VALUE_LONG, NULL },
  { "UPDATETIME",           TAG_KEY_UPDATEFLAG,           VALUE_LONG, NULL },
  { "VARIOUSARTISTS",       TAG_KEY_VOLUMEADJUSTPERC,     VALUE_LONG, NULL },
  { "VOLUMEADJUSTPERC",     TAG_KEY_WRITETIME,            VALUE_LONG, NULL },
  { "WRITETIME",            TAG_KEY_UPDATETIME,           VALUE_LONG, NULL },
};
#define SONG_DFKEY_COUNT (sizeof (songdfkeys) / sizeof (datafilekey_t))

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
      llistFree (song->songInfo);
    }
    free (song);
  }
}

void
songParse (song_t *song, char *data)
{
  song->songInfo = datafileParse (data, "song", DFTYPE_KEY_VAL,
      songdfkeys, SONG_DFKEY_COUNT, DATAFILE_NO_LOOKUP, NULL);
  llistSort (song->songInfo);
}

char *
songGetData (song_t *song, long idx) {
  if (song == NULL || song->songInfo == NULL) {
    return NULL;
  }

  char *value = llistGetData (song->songInfo, idx);
  return value;
}

long
songGetLong (song_t *song, long idx) {
  if (song == NULL || song->songInfo == NULL) {
    return -1L;
  }

  long value = llistGetLong (song->songInfo, idx);
  return value;
}

void
songSetLong (song_t *song, long tagidx, long value)
{
  if (song == NULL || song->songInfo == NULL) {
    return;
  }

  llistSetLong (song->songInfo, tagidx, value);
}

