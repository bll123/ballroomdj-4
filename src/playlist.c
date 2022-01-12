#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdjvarsdf.h"
#include "dance.h"
#include "datafile.h"
#include "datautil.h"
#include "fileop.h"
#include "level.h"
#include "log.h"
#include "musicdb.h"
#include "playlist.h"
#include "portability.h"
#include "rating.h"
#include "sequence.h"
#include "song.h"
#include "songlist.h"
#include "songsel.h"
#include "status.h"
#include "tagdef.h"

static void     plConvResume (char *, datafileret_t *ret);
static void     plConvWait (char *, datafileret_t *ret);
static void     plConvStopType (char *, datafileret_t *ret);
static void     plConvType (char *, datafileret_t *ret);

  /* must be sorted in ascii order */
static datafilekey_t playlistdfkeys[] = {
  { "ALLOWEDKEYWORDS",  PLAYLIST_ALLOWED_KEYWORDS,
      VALUE_DATA, parseConvTextList },
  { "DANCELEVELHIGH",   PLAYLIST_LEVEL_HIGH,
      VALUE_DATA, levelConv },
  { "DANCELEVELLOW",    PLAYLIST_LEVEL_LOW,
      VALUE_DATA, levelConv },
  { "DANCERATING",      PLAYLIST_RATING,
      VALUE_DATA, ratingConv },
  { "GAP",              PLAYLIST_GAP,
      VALUE_DATA, NULL },
  { "MANUALLIST",       PLAYLIST_MANUAL_LIST_NAME,
      VALUE_DATA, NULL },
  { "MAXPLAYTIME",      PLAYLIST_MAX_PLAY_TIME,
      VALUE_LONG, NULL },
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
  { "MAXPLAYTIME",    PLDANCE_MAXPLAYTIME,  VALUE_LONG, NULL },
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
    logMsg (LOG_DBG, LOG_IMPORTANT, "Missing playlist-pl %s", tfn);
    return NULL;
  }

  pl = malloc (sizeof (playlist_t));
  assert (pl != NULL);
  pl->manualIdx = 0;
  pl->plinfodf = NULL;
  pl->pldancesdf = NULL;
  pl->songlist = NULL;
  pl->sequence = NULL;
  pl->songsel = NULL;
  pl->plinfo = NULL;
  pl->pldances = NULL;

  pl->plinfodf = datafileAllocParse ("playlist", DFTYPE_KEY_VAL, tfn,
      playlistdfkeys, PLAYLIST_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  if (! fileopExists (tfn)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "Bad playlist-pl %s", tfn);
    playlistFree (pl);
    return NULL;
  }
  pl->plinfo = datafileGetList (pl->plinfodf);
  llistDumpInfo (pl->plinfo);

  datautilMakePath (tfn, sizeof (tfn), "", fname, ".pldances", DATAUTIL_MP_NONE);
  if (! fileopExists (tfn)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "Missing playlist-dance %s", tfn);
    playlistFree (pl);
    return NULL;
  }

  pl->pldancesdf = datafileAllocParse ("playlist-dances", DFTYPE_INDIRECT, tfn,
      playlistdancedfkeys, PLAYLIST_DANCE_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  if (pl->pldancesdf == NULL) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "Bad playlist-dance %s", tfn);
    playlistFree (pl);
    return NULL;
  }
  pl->pldances = datafileGetList (pl->pldancesdf);
  llistDumpInfo (pl->pldances);

  type = (pltype_t) llistGetLong (pl->plinfo, PLAYLIST_TYPE);

  if (type == PLTYPE_MANUAL) {
    char *slfname = llistGetData (pl->plinfo, PLAYLIST_MANUAL_LIST_NAME);
    datautilMakePath (tfn, sizeof (tfn), "", slfname, ".songlist", DATAUTIL_MP_NONE);
    if (! fileopExists (tfn)) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "Missing songlist %s", tfn);
      playlistFree (pl);
      return NULL;
    }
    pl->songlist = songlistAlloc (tfn);
    if (pl->songlist == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "Bad songlist %s", tfn);
      playlistFree (pl);
      return NULL;
    }
  }

  if (type == PLTYPE_SEQ) {
    char *seqfname = llistGetData (pl->plinfo, PLAYLIST_SEQ_NAME);
    datautilMakePath (tfn, sizeof (tfn), "", seqfname, ".sequence", DATAUTIL_MP_NONE);
    if (! fileopExists (tfn)) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "Missing sequence %s", tfn);
      playlistFree (pl);
      return NULL;
    }
    pl->sequence = sequenceAlloc (seqfname);
    if (pl->sequence == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "Bad sequence %s", seqfname);
      playlistFree (pl);
      return NULL;
    }
    sequenceStartIterator (pl->sequence);
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
    if (pl->pldancesdf != NULL) {
      datafileFree (pl->pldancesdf);
    }
    if (pl->songlist != NULL) {
      songlistFree (pl->songlist);
    }
    if (pl->sequence != NULL) {
      sequenceFree (pl->sequence);
    }
    if (pl->songsel != NULL) {
      songselFree (pl->songsel);
    }
    free (pl);
  }
}

song_t *
playlistGetNextSong (playlist_t *pl)
{
  pltype_t    type;
  song_t      *song = NULL;

  if (pl == NULL) {
    return NULL;
  }

  type = (pltype_t) llistGetLong (pl->plinfo, PLAYLIST_TYPE);

  switch (type) {
    case PLTYPE_AUTO: {
      break;
    }
    case PLTYPE_MANUAL: {
      char *sfname;

      sfname = songlistGetData (pl->songlist, pl->manualIdx, SONGLIST_FILE);
      while (sfname != NULL) {
        song = dbGetByName (sfname);
        if (song != NULL && songAudioFileExists (song)) {
          break;
        }
        song = NULL;
        logMsg (LOG_DBG, LOG_IMPORTANT, "manual: missing: %s", sfname);
        ++pl->manualIdx;
        sfname = songlistGetData (pl->songlist, pl->manualIdx, SONGLIST_FILE);
      }
      ++pl->manualIdx;
      logMsg (LOG_DBG, LOG_BASIC, "manual: get: %s", sfname);
      break;
    }
    case PLTYPE_SEQ: {
      if (pl->songsel == NULL) {
        pl->songsel = songselAlloc (sequenceGetDanceList (pl->sequence),
            playlistFilterSong, pl);
      }
      long dancekey = sequenceIterate (pl->sequence);
      song = songselSelect (pl->songsel, dancekey);
      break;
    }
  }
  return song;
}

bool
playlistFilterSong (song_t *song, void *tplaylist)
{
  playlist_t    *pl = tplaylist;
  long          rating;
  long          plRating;
  long          plLevelLow;
  long          plLevelHigh;
  long          level;
  char          *keyword;
  list_t        *kwList;
  long          idx;
  status_t      *status;
  long          sstatus;


  plRating = llistGetLong (pl->plinfo, PLAYLIST_RATING);
  rating = songGetLong (song, TAG_DANCERATING);
  if (rating < plRating) {
    logMsg (LOG_DBG, LOG_SONGSEL, "rating %ld < %ld", rating, plRating);
    return false;
  }

  plLevelLow = llistGetLong (pl->plinfo, PLAYLIST_LEVEL_LOW);
  plLevelHigh = llistGetLong (pl->plinfo, PLAYLIST_LEVEL_HIGH);
  level = songGetLong (song, TAG_DANCELEVEL);
  if (level < plLevelLow || level > plLevelHigh) {
    logMsg (LOG_DBG, LOG_SONGSEL, "level %ld < %ld / > %ld", level, plLevelLow, plLevelHigh);
    return false;
  }

  keyword = songGetData (song, TAG_KEYWORD);
  if (keyword != NULL) {
    kwList = llistGetList (pl->plinfo, PLAYLIST_ALLOWED_KEYWORDS);
    idx = listGetStrIdx (kwList, keyword);
    if (listGetSize (kwList) > 0 && idx < 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "keyword %s not in allowed", keyword);
      return false;
    }
  }

  kwList = llistGetList (pl->plinfo, PLAYLIST_REQ_KEYWORDS);
  if (listGetSize (kwList) > 0) {
    if (keyword == NULL) {
      logMsg (LOG_DBG, LOG_SONGSEL, "keyword empty", keyword);
      return false;
    }
    idx = listGetStrIdx (kwList, keyword);
    if (idx < 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "keyword not in required", keyword);
      return false;
    }
  }

  if (llistGetLong (pl->plinfo, PLAYLIST_USE_STATUS)) {
    sstatus = songGetLong (song, TAG_STATUS);
    status = bdjvarsdf [BDJVDF_STATUS];
    if (status != NULL && ! statusPlayCheck (status, sstatus)) {
      logMsg (LOG_DBG, LOG_SONGSEL, "status %ld not playable", status);
      return false;
    }
  }

  return true;
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

