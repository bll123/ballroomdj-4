#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "playlist.h"
#include "datafile.h"
#include "fileop.h"
#include "portability.h"
#include "datautil.h"
#include "rating.h"
#include "level.h"
#include "dance.h"
#include "song.h"
#include "songlist.h"
#include "sequence.h"
#include "log.h"

static void     plConvResume (char *, datafileret_t *ret);
static void     plConvWait (char *, datafileret_t *ret);
static void     plConvStopType (char *, datafileret_t *ret);
static void     plConvType (char *, datafileret_t *ret);

  /* must be sorted in ascii order */
static datafilekey_t playlistdfkeys[] = {
  { "ALLOWEDKEYWORDS",  PLAYLIST_ALLOWED_KEYWORDS,
      VALUE_DATA, parseConvTextList },
  { "DANCERATING",      PLAYLIST_RATING,
      VALUE_DATA, ratingConv },
  { "GAP",              PLAYLIST_GAP,
      VALUE_DATA, NULL },
  { "HIGHDANCELEVEL",   PLAYLIST_LEVEL_LOW,
      VALUE_DATA, levelConv },
  { "LOWDANCELEVEL",    PLAYLIST_LEVEL_HIGH,
      VALUE_DATA, levelConv },
  { "MANUALLIST",       PLAYLIST_MANUAL_LIST_NAME,
      VALUE_DATA, NULL },
  { "MAXPLAYTIME",      PLAYLIST_MAX_PLAY_TIME,
      VALUE_DATA, NULL },
  { "MQMESSAGE",        PLAYLIST_MQ_MESSAGE,
      VALUE_DATA, NULL },
  { "PAUSEEACHSONG",    PLAYLIST_PAUSE_EACH_SONG,
      VALUE_LONG, parseConvBoolean },
  { "PLAYANNOUNCE",     PLAYLIST_ANNOUNCE,
      VALUE_LONG, parseConvBoolean },
  { "REQUIREDKEYWORDS", PLAYLIST_REQ_KEYWORDS,
      VALUE_DATA, parseConvTextList },
  { "RESUME",           PLAYLIST_RESUME,
      VALUE_DATA, plConvResume },
  { "SEQUENCE",         PLAYLIST_SEQ_NAME,
      VALUE_DATA, NULL },
  { "STOPAFTER",        PLAYLIST_STOP_AFTER,
      VALUE_DATA, plConvWait },
  { "STOPAFTERWAIT",    PLAYLIST_STOP_AFTER_WAIT,
      VALUE_DATA, plConvWait },
  { "STOPTIME",         PLAYLIST_STOP_TIME,
      VALUE_DATA, NULL },
  { "STOPTYPE",         PLAYLIST_STOP_TYPE,
      VALUE_DATA, plConvStopType },
  { "STOPWAIT",         PLAYLIST_STOP_WAIT,
      VALUE_DATA, plConvWait },
  { "TYPE",             PLAYLIST_TYPE,
      VALUE_DATA, plConvType },
  { "USESTATUS",        PLAYLIST_USE_STATUS,
      VALUE_DATA, parseConvBoolean },
  { "USEUNRATED",       PLAYLIST_USE_UNRATED,
      VALUE_DATA, parseConvBoolean },
};
#define PLAYLIST_DFKEY_COUNT (sizeof (playlistdfkeys) / sizeof (datafilekey_t))

  /* must be sorted in ascii order */
static datafilekey_t playlistdancedfkeys[] = {
  { "COUNT",          PLDANCE_COUNT,        VALUE_LONG, NULL },
  { "DANCE",          PLDANCE_DANCE,        VALUE_DATA, danceConvDance },
  { "HIGHBPM",        PLDANCE_HIGHBPM,      VALUE_LONG, NULL },
  { "LOWBPM",         PLDANCE_LOWBPM,       VALUE_LONG, NULL },
  { "MAXPLAYTIME",    PLDANCE_MAXPLAYTIME,  VALUE_DATA, NULL },
  { "SELECTED",       PLDANCE_SELECTED,     VALUE_LONG, parseConvBoolean },
};
#define PLAYLIST_DANCE_DFKEY_COUNT (sizeof (playlistdancedfkeys) / sizeof (datafilekey_t))

playlist_t *
playlistAlloc (char *fname)
{
  playlist_t    *pl = NULL;
  char          tfn [MAXPATHLEN];
  pltype_t      type;

  datautilMakePath (tfn, sizeof (tfn), "", fname, ".pl", DATAUTIL_MP_NONE);
  if (! fileopExists (tfn)) {
    logMsg (LOG_DBG, LOG_LVL_1, "Missing playlist-pl %s\n", tfn);
    return NULL;
  }

  pl = malloc (sizeof (playlist_t));
  assert (pl != NULL);
  pl->sequenceIdx = 0;
  pl->manualIdx = 0;
  pl->plinfodf = NULL;
  pl->dancesdf = NULL;
  pl->songlistdf = NULL;
  pl->seqdf = NULL;
  pl->plinfo = NULL;
  pl->dances = NULL;
  pl->songlist = NULL;
  pl->seq = NULL;

  pl->plinfodf = datafileAllocParse ("playlist", DFTYPE_KEY_VAL, tfn,
      playlistdfkeys, PLAYLIST_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  if (! fileopExists (tfn)) {
    logMsg (LOG_DBG, LOG_LVL_1, "Bad playlist-pl %s\n", tfn);
    playlistFree (pl);
    return NULL;
  }
  pl->plinfo = datafileGetList (pl->plinfodf);

  datautilMakePath (tfn, sizeof (tfn), "", fname, ".pldances", DATAUTIL_MP_NONE);
  if (! fileopExists (tfn)) {
    logMsg (LOG_DBG, LOG_LVL_1, "Missing playlist-dance %s\n", tfn);
    playlistFree (pl);
    return NULL;
  }

  pl->dancesdf = datafileAllocParse ("playlist-dances", DFTYPE_KEY_LONG, tfn,
      playlistdancedfkeys, PLAYLIST_DANCE_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  if (pl->dancesdf == NULL) {
    logMsg (LOG_DBG, LOG_LVL_1, "Bad playlist-dance %s\n", tfn);
    playlistFree (pl);
    return NULL;
  }
  pl->dances = datafileGetList (pl->dancesdf);

  type = llistGetLong (pl->plinfo, PLAYLIST_TYPE);

  if (type == PLTYPE_MANUAL) {
    char *slfname = llistGetData (pl->plinfo, PLAYLIST_MANUAL_LIST_NAME);
    datautilMakePath (tfn, sizeof (tfn), "", slfname, ".songlist", DATAUTIL_MP_NONE);
    if (! fileopExists (tfn)) {
      logMsg (LOG_DBG, LOG_LVL_1, "Missing songlist %s\n", tfn);
      playlistFree (pl);
      return NULL;
    }
    pl->songlistdf = songlistAlloc (tfn);
    if (pl->songlistdf == NULL) {
      logMsg (LOG_DBG, LOG_LVL_1, "Bad songlist %s\n", tfn);
      playlistFree (pl);
      return NULL;
    }
    pl->songlist = datafileGetList (pl->songlistdf);
  }

  if (type == PLTYPE_SEQ) {
    char *seqfname = llistGetData (pl->plinfo, PLAYLIST_SEQ_NAME);
    datautilMakePath (tfn, sizeof (tfn), "", seqfname, ".sequence", DATAUTIL_MP_NONE);
    if (! fileopExists (tfn)) {
      logMsg (LOG_DBG, LOG_LVL_1, "Missing sequence %s\n", tfn);
      playlistFree (pl);
      return NULL;
    }
    pl->seqdf = sequenceAlloc (seqfname);
    if (pl->seqdf == NULL) {
      logMsg (LOG_DBG, LOG_LVL_1, "Bad sequence %s\n", seqfname);
      playlistFree (pl);
      return NULL;
    }
    pl->seq = datafileGetList (pl->seqdf);
  }

  return pl;
}

void
playlistFree (playlist_t *pl)
{
  if (pl != NULL) {
    if (pl->plinfodf != NULL) {
      datafileFree (pl->plinfodf);
    }
    if (pl->dancesdf != NULL) {
      datafileFree (pl->dancesdf);
    }
    if (pl->songlistdf != NULL) {
      songlistFree (pl->songlistdf);
    }
    if (pl->seqdf != NULL) {
      sequenceFree (pl->seqdf);
    }
    free (pl);
  }
}

song_t *
playlistGetNextSong (playlist_t *pl)
{
  pltype_t    type;
  song_t      *song = NULL;

  type = llistGetLong (pl->plinfo, PLAYLIST_TYPE);

  switch (type) {
    case PLTYPE_AUTO: {
      break;
    }
    case PLTYPE_MANUAL: {
        /* manual playlists are easy... */
      break;
    }
    case PLTYPE_SEQ: {
      break;
    }
  }
  return song;
}

/* internal routines */

static void
plConvResume (char *data, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  ret->u.l = RESUME_FROM_LAST;
  if (strcmp (data, "From Start") == 0) {
    ret->u.l = RESUME_FROM_START;
  }
}

static void
plConvWait (char *data, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  ret->u.l = WAIT_CONTINUE;
  if (strcmp (data, "") == 0) {
    ret->u.l = WAIT_PAUSE;
  }
}

static void
plConvStopType (char *data, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  ret->u.l = STOP_AT;
  if (strcmp (data, "In") == 0) {
    ret->u.l = STOP_IN;
  }
}

static void
plConvType (char *data, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  ret->u.l = PLTYPE_MANUAL;
  if (strcmp (data, "Automatic") == 0) {
    ret->u.l = PLTYPE_AUTO;
  }
  if (strcmp (data, "Sequence") == 0) {
    ret->u.l = PLTYPE_SEQ;
  }
}

