#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjopt.h"
#include "bdjstring.h"
#include "dance.h"
#include "datafile.h"
#include "fileop.h"
#include "genre.h"
#include "level.h"
#include "list.h"
#include "log.h"
#include "portability.h"
#include "rating.h"
#include "song.h"
#include "status.h"
#include "tagdef.h"

  /* must be sorted in ascii order */
static datafilekey_t songdfkeys[] = {
  { "ADJUSTFLAGS",          TAG_ADJUSTFLAGS,          VALUE_DATA, NULL },
  { "AFMODTIME",            TAG_AFMODTIME,            VALUE_NUM, NULL },
  { "ALBUM",                TAG_ALBUM,                VALUE_DATA, NULL },
  { "ALBUMARTIST",          TAG_ALBUMARTIST,          VALUE_DATA, NULL },
  { "ARTIST",               TAG_ARTIST,               VALUE_DATA, NULL },
  { "AUTOORGFLAG",          TAG_AUTOORGFLAG,          VALUE_NUM, NULL },
  { "BPM",                  TAG_BPM,                  VALUE_NUM, NULL },
  { "COMPOSER",             TAG_COMPOSER,             VALUE_DATA, NULL },
  { "CONDUCTOR",            TAG_CONDUCTOR,            VALUE_DATA, NULL },
  { "DANCE",                TAG_DANCE,                VALUE_NUM, danceConvDance },
  { "DANCELEVEL",           TAG_DANCELEVEL,           VALUE_DATA, levelConv },
  { "DANCERATING",          TAG_DANCERATING,          VALUE_DATA, ratingConv },
  { "DATE",                 TAG_DATE,                 VALUE_DATA, NULL },
  { "DBADDDATE",            TAG_DBADDDATE,            VALUE_DATA, NULL },
  { "DISCNUMBER",           TAG_DISCNUMBER,           VALUE_NUM, NULL },
  { "DISCTOTAL",            TAG_DISCTOTAL,            VALUE_NUM, NULL },
  { "DISPLAYIMG",           TAG_DISPLAYIMG,           VALUE_DATA, NULL },
  { "DURATION",             TAG_DURATION,             VALUE_NUM, NULL },
  { "FILE",                 TAG_FILE,                 VALUE_DATA, NULL },
  { "GENRE",                TAG_GENRE,                VALUE_DATA, genreConv },
  { "KEYWORD",              TAG_KEYWORD,              VALUE_DATA, NULL },
  { "MQDISPLAY",            TAG_MQDISPLAY,            VALUE_DATA, NULL },
  { "MUSICBRAINZ_TRACKID",  TAG_MUSICBRAINZ_TRACKID,  VALUE_DATA, NULL },
  { "NOMAXPLAYTIME",        TAG_NOMAXPLAYTIME,        VALUE_NUM, parseConvBoolean },
  { "NOTES",                TAG_NOTES,                VALUE_DATA, NULL },
  { "RRN",                  TAG_RRN,                  VALUE_NUM, NULL },
  { "SAMESONG",             TAG_SAMESONG,             VALUE_DATA, NULL },
  { "SONGEND",              TAG_SONGEND,              VALUE_NUM, NULL },
  { "SONGSTART",            TAG_SONGSTART,            VALUE_NUM, NULL },
  { "SPEEDADJUSTMENT",      TAG_SPEEDADJUSTMENT,      VALUE_NUM, NULL },
  { "STATUS",               TAG_STATUS,               VALUE_NUM, statusConv },
  { "TAGS",                 TAG_TAGS,                 VALUE_LIST, parseConvTextList },
  { "TITLE",                TAG_TITLE,                VALUE_DATA, NULL },
  { "TRACKNUMBER",          TAG_TRACKNUMBER,          VALUE_NUM, NULL },
  { "TRACKTOTAL",           TAG_TRACKTOTAL,           VALUE_NUM, NULL },
  { "UPDATEFLAG",           TAG_VARIOUSARTISTS,       VALUE_NUM, NULL },
  { "UPDATETIME",           TAG_UPDATEFLAG,           VALUE_NUM, NULL },
  { "VARIOUSARTISTS",       TAG_VOLUMEADJUSTPERC,     VALUE_NUM, NULL },
  { "VOLUMEADJUSTPERC",     TAG_WRITETIME,            VALUE_NUM, NULL },
  { "WRITETIME",            TAG_UPDATETIME,           VALUE_NUM, NULL },
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
songGetData (song_t *song, listidx_t idx)
{
  char    *value;

  if (song == NULL || song->songInfo == NULL) {
    return NULL;
  }

  value = llistGetData (song->songInfo, idx);
  return value;
}

ssize_t
songGetNum (song_t *song, listidx_t idx)
{
  ssize_t     value;

  if (song == NULL || song->songInfo == NULL) {
    return -1L;
  }

  value = llistGetNum (song->songInfo, idx);
  return value;
}

double
songGetDouble (song_t *song, listidx_t idx)
{
  double      value;


  if (song == NULL || song->songInfo == NULL) {
    return -1;
  }

  value = llistGetDouble (song->songInfo, idx);
  return value;
}

void
songSetNum (song_t *song, listidx_t tagidx, ssize_t value)
{
  if (song == NULL || song->songInfo == NULL) {
    return;
  }

  llistSetNum (song->songInfo, tagidx, value);
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

