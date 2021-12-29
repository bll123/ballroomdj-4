#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "playlist.h"
#include "datafile.h"
#include "fileutil.h"

static long      plConvResume (const char *);
static long      plConvWait (const char *);
static long      plConvStopType (const char *);

static datafilekey_t playlistdfkeys[] = {
  { "AllowedKeywords",   PLAYLIST_ALLOWED_KEYWORDS,
      VALUE_DATA, NULL },
  { "DanceRating",       PLAYLIST_RATING,
      VALUE_DATA, NULL },
  { "ManualList",        PLAYLIST_MANUAL_LIST_NAME,
      VALUE_DATA, NULL },
  { "MaxPlayTime",       PLAYLIST_MAX_PLAY_TIME,
      VALUE_DATA, NULL },
  { "PauseEachSong",     PLAYLIST_PAUSE_EACH_SONG,
      VALUE_LONG, parseConvBoolean },
  { "PlayAnnounce",      PLAYLIST_ANNOUNCE,
      VALUE_LONG, parseConvBoolean },
  { "RequiredKeywords",  PLAYLIST_REQ_KEYWORDS,
      VALUE_DATA, NULL },
  { "Resume",            PLAYLIST_RESUME,
      VALUE_DATA, plConvResume },
  { "Sequence",          PLAYLIST_SEQ_NAME,
      VALUE_DATA, NULL },
  { "StopAfterWait",     PLAYLIST_STOP_AFTER_WAIT,
      VALUE_DATA, plConvWait },
  { "StopTime",          PLAYLIST_STOP_TIME,
      VALUE_DATA, NULL },
  { "StopType",          PLAYLIST_STOP_TYPE,
      VALUE_DATA, plConvStopType },
  { "StopWait",          PLAYLIST_STOP_WAIT,
      VALUE_DATA, plConvWait },
  { "UseStatus",         PLAYLIST_USE_STATUS,
      VALUE_DATA, parseConvBoolean },
  { "UseUnrated",        PLAYLIST_USE_UNRATED,
      VALUE_DATA, parseConvBoolean },
  { "gap",               PLAYLIST_GAP,
      VALUE_DATA, NULL },
  { "highDanceLevel",    PLAYLIST_LEVEL_LOW,
      VALUE_DATA, NULL },
  { "lowDanceLevel",     PLAYLIST_LEVEL_HIGH,
      VALUE_DATA, NULL },
  { "mqMessage",         PLAYLIST_MQ_MESSAGE,
      VALUE_DATA, NULL },
};
#define PLAYLIST_DFKEY_COUNT (sizeof (playlistdfkeys) / sizeof (datafilekey_t))

datafile_t *
playlistAlloc (char *fname)
{
  datafile_t    *df;

  if (! fileExists (fname)) {
    return NULL;
  }
  df = datafileAlloc (playlistdfkeys, PLAYLIST_DFKEY_COUNT, fname, DFTYPE_KEY_VAL);

  // rebuild allowed keywords
  // rebuild required keywords
  // convert high level
  // convert low level

  // handle dances; this may require a re-parse?
  // have datafile save the parse???

  return df;
}

void
playlistFree (datafile_t *df)
{
  datafileFree (df);
}

/* internal routines */

long
plConvResume (const char *data)
{
  long val = RESUME_FROM_LAST;
  if (strcmp (data, "From Start") == 0) {
    val = RESUME_FROM_START;
  }
  return val;
}

long
plConvWait (const char *data)
{
  long val = WAIT_CONTINUE;
  if (strcmp (data, "") == 0) {
    val = WAIT_PAUSE;
  }
  return val;
}

long
plConvStopType (const char *data)
{
  long val = STOP_AT;
  if (strcmp (data, "In") == 0) {
    val = STOP_IN;
  }
  return val;
}

