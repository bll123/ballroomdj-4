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
#include "filemanip.h"
#include "fileop.h"
#include "ilist.h"
#include "level.h"
#include "log.h"
#include "nlist.h"
#include "musicdb.h"
#include "pathutil.h"
#include "playlist.h"
#include "portability.h"
#include "rating.h"
#include "sequence.h"
#include "slist.h"
#include "song.h"
#include "songlist.h"
#include "songsel.h"
#include "status.h"
#include "tagdef.h"

static void     plConvType (char *, datafileret_t *ret);

  /* must be sorted in ascii order */
static datafilekey_t playlistdfkeys[] = {
  { "ALLOWEDKEYWORDS",  PLAYLIST_ALLOWED_KEYWORDS,
      VALUE_DATA, parseConvTextList },
  { "DANCELEVELHIGH",   PLAYLIST_LEVEL_HIGH,
      VALUE_NUM, levelConv },
  { "DANCELEVELLOW",    PLAYLIST_LEVEL_LOW,
      VALUE_NUM, levelConv },
  { "DANCERATING",      PLAYLIST_RATING,
      VALUE_NUM, ratingConv },
  { "GAP",              PLAYLIST_GAP,
      VALUE_NUM, NULL },
  { "MANUALLIST",       PLAYLIST_MANUAL_LIST_NAME,
      VALUE_DATA, NULL },
  { "MAXPLAYTIME",      PLAYLIST_MAX_PLAY_TIME,
      VALUE_NUM, NULL },
  { "MQMESSAGE",        PLAYLIST_MQ_MESSAGE,
      VALUE_DATA, NULL },
  { "PAUSEEACHSONG",    PLAYLIST_PAUSE_EACH_SONG,
      VALUE_NUM, parseConvBoolean },
  { "PLAYANNOUNCE",     PLAYLIST_ANNOUNCE,
      VALUE_NUM, parseConvBoolean },
  { "SEQUENCE",         PLAYLIST_SEQ_NAME,
      VALUE_DATA, NULL },
  { "TYPE",             PLAYLIST_TYPE,
      VALUE_NUM, plConvType },
  { "USESTATUS",        PLAYLIST_USE_STATUS,
      VALUE_NUM, parseConvBoolean },
};
#define PLAYLIST_DFKEY_COUNT (sizeof (playlistdfkeys) / sizeof (datafilekey_t))

  /* must be sorted in ascii order */
static datafilekey_t playlistdancedfkeys[] = {
  { "BPMHIGH",        PLDANCE_BPM_HIGH,     VALUE_NUM, NULL },
  { "BPMLOW",         PLDANCE_BPM_LOW,      VALUE_NUM, NULL },
  { "COUNT",          PLDANCE_COUNT,        VALUE_NUM, NULL },
  { "DANCE",          PLDANCE_DANCE,        VALUE_DATA, danceConvDance },
  { "MAXPLAYTIME",    PLDANCE_MAXPLAYTIME,  VALUE_NUM, NULL },
  { "SELECTED",       PLDANCE_SELECTED,     VALUE_NUM, parseConvBoolean },
};
#define PLAYLIST_DANCE_DFKEY_COUNT (sizeof (playlistdancedfkeys) / sizeof (datafilekey_t))

playlist_t *
playlistLoad (char *fname)
{
  playlist_t    *pl = NULL;
  char          tfn [MAXPATHLEN];
  pltype_t      type;


  datautilMakePath (tfn, sizeof (tfn), "", fname, ".pl", DATAUTIL_MP_NONE);
  if (! fileopExists (tfn)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Missing playlist-pl %s", tfn);
    return NULL;
  }

  pl = playlistAlloc (fname);

  pl->plinfodf = datafileAllocParse ("playlist-pl", DFTYPE_KEY_VAL, tfn,
      playlistdfkeys, PLAYLIST_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  if (! fileopExists (tfn)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Bad playlist-pl %s", tfn);
    playlistFree (pl);
    return NULL;
  }
  pl->plinfo = datafileGetList (pl->plinfodf);
  nlistDumpInfo (pl->plinfo);

  datautilMakePath (tfn, sizeof (tfn), "", fname, ".pldances", DATAUTIL_MP_NONE);
  if (! fileopExists (tfn)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Missing playlist-dance %s", tfn);
    playlistFree (pl);
    return NULL;
  }

  pl->pldancesdf = datafileAllocParse ("playlist-dances", DFTYPE_INDIRECT, tfn,
      playlistdancedfkeys, PLAYLIST_DANCE_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  if (pl->pldancesdf == NULL) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Bad playlist-dance %s", tfn);
    playlistFree (pl);
    return NULL;
  }
  pl->pldances = datafileGetList (pl->pldancesdf);
  ilistDumpInfo (pl->pldances);

  type = (pltype_t) nlistGetNum (pl->plinfo, PLAYLIST_TYPE);

  if (type == PLTYPE_MANUAL) {
    char *slfname = nlistGetStr (pl->plinfo, PLAYLIST_MANUAL_LIST_NAME);
    datautilMakePath (tfn, sizeof (tfn), "", slfname, ".songlist", DATAUTIL_MP_NONE);
    if (! fileopExists (tfn)) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Missing songlist %s", tfn);
      playlistFree (pl);
      return NULL;
    }
    pl->songlist = songlistAlloc (tfn);
    if (pl->songlist == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Bad songlist %s", tfn);
      playlistFree (pl);
      return NULL;
    }
  }

  if (type == PLTYPE_SEQ) {
    char *seqfname = nlistGetStr (pl->plinfo, PLAYLIST_SEQ_NAME);
    datautilMakePath (tfn, sizeof (tfn), "", seqfname, ".sequence", DATAUTIL_MP_NONE);
    if (! fileopExists (tfn)) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Missing sequence %s", tfn);
      playlistFree (pl);
      return NULL;
    }
    pl->sequence = sequenceAlloc (seqfname);
    if (pl->sequence == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Bad sequence %s", seqfname);
      playlistFree (pl);
      return NULL;
    }
    sequenceStartIterator (pl->sequence);
  }

  return pl;
}

playlist_t *
playlistCreate (char *plfname, pltype_t type, char *ofname)
{
  playlist_t    *pl = NULL;
  char          tbuff [40];
  ilistidx_t    dkey;
  ilist_t       *dl;
  ssize_t       idx;


  pl = playlistAlloc (plfname);

  snprintf (tbuff, sizeof (tbuff), "plinfo-%s", plfname);
  pl->plinfo = nlistAlloc (tbuff, LIST_UNORDERED, free);
  nlistSetSize (pl->plinfo, PLAYLIST_KEY_MAX);
  nlistSetStr (pl->plinfo, PLAYLIST_ALLOWED_KEYWORDS, NULL);
  nlistSetNum (pl->plinfo, PLAYLIST_ANNOUNCE, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_GAP, 1000);
  nlistSetNum (pl->plinfo, PLAYLIST_LEVEL_HIGH, LIST_VALUE_INVALID);
  nlistSetNum (pl->plinfo, PLAYLIST_LEVEL_LOW, 0);
  nlistSetStr (pl->plinfo, PLAYLIST_MANUAL_LIST_NAME, NULL);
  nlistSetNum (pl->plinfo, PLAYLIST_MAX_PLAY_TIME, 0);
  nlistSetStr (pl->plinfo, PLAYLIST_MQ_MESSAGE, NULL);
  nlistSetNum (pl->plinfo, PLAYLIST_PAUSE_EACH_SONG, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_RATING, 0);
  nlistSetStr (pl->plinfo, PLAYLIST_SEQ_NAME, NULL);
  nlistSetNum (pl->plinfo, PLAYLIST_TYPE, type);

  if (ofname == NULL) {
    ofname = plfname;
  }
  if (type == PLTYPE_MANUAL) {
    nlistSetStr (pl->plinfo, PLAYLIST_MANUAL_LIST_NAME, strdup (ofname));
    pl->songlist = songlistAlloc (ofname);
  }
  if (type == PLTYPE_SEQ) {
    nlistSetStr (pl->plinfo, PLAYLIST_SEQ_NAME, strdup (ofname));
    pl->sequence = sequenceAlloc (ofname);
  }

  snprintf (tbuff, sizeof (tbuff), "pldance-%s", plfname);
  pl->pldances = ilistAlloc (tbuff, LIST_ORDERED, free);
  idx = 0;
  dl = danceGetDanceList (bdjvarsdf [BDJVDF_DANCES]);
  ilistStartIterator (dl);
  while ((dkey = ilistIterateKey (dl)) != LIST_VALUE_INVALID) {
    snprintf (tbuff, sizeof (tbuff), "pldance-%s-%zd", plfname, dkey);
    ilistSetNum (pl->pldances, idx, PLDANCE_BPM_HIGH, LIST_VALUE_INVALID);
    ilistSetNum (pl->pldances, idx, PLDANCE_BPM_LOW, LIST_VALUE_INVALID);
    ilistSetNum (pl->pldances, idx, PLDANCE_COUNT, 0);
    ilistSetNum (pl->pldances, idx, PLDANCE_DANCE, dkey);
    ilistSetNum (pl->pldances, idx, PLDANCE_MAXPLAYTIME, LIST_VALUE_INVALID);
    ilistSetNum (pl->pldances, idx, PLDANCE_SELECTED, 0);
    ++idx;
  }
  listFree (dl);

  return pl;
}

playlist_t *
playlistAlloc (char *fname)
{
  playlist_t    *pl = NULL;

  pl = malloc (sizeof (playlist_t));
  assert (pl != NULL);
  pl->name = strdup (fname);
  pl->manualIdx = 0;
  pl->plinfodf = NULL;
  pl->pldancesdf = NULL;
  pl->songlist = NULL;
  pl->sequence = NULL;
  pl->songsel = NULL;
  pl->plinfo = NULL;
  pl->pldances = NULL;

  return pl;
}

void
playlistFree (void *tpl)
{
  playlist_t      *pl = tpl;

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
    if (pl->name != NULL) {
      free (pl->name);
    }
    free (pl);
  }
}

char *
playlistGetName (playlist_t *pl)
{
  if (pl == NULL) {
    return NULL;
  }

  return pl->name;
}

ssize_t
playlistGetConfigNum (playlist_t *pl, playlistkey_t key)
{
  ssize_t     val;

  if (pl == NULL || pl->plinfo == NULL) {
    return LIST_VALUE_INVALID;
  }

  val = nlistGetNum (pl->plinfo, key);
  return val;
}

ssize_t
playlistGetDanceNum (playlist_t *pl, dancekey_t dancekey, pldancekey_t key)
{
  ssize_t     val;

  if (pl == NULL || pl->plinfo == NULL) {
    return LIST_VALUE_INVALID;
  }

  val = ilistGetNum (pl->pldances, dancekey, key);
  return val;
}

song_t *
playlistGetNextSong (playlist_t *pl, playlistCheck_t checkProc, void *userdata)
{
  pltype_t    type;
  song_t      *song = NULL;
  int         count;
  char        *sfname;


  if (pl == NULL) {
    return NULL;
  }

  type = (pltype_t) nlistGetNum (pl->plinfo, PLAYLIST_TYPE);

  switch (type) {
    case PLTYPE_AUTO: {
      break;
    }
    case PLTYPE_MANUAL: {
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
      logMsg (LOG_DBG, LOG_BASIC, "manual: select: %s", sfname);
      break;
    }
    case PLTYPE_SEQ: {
      ilistidx_t     danceIdx;

      if (pl->songsel == NULL) {
        pl->songsel = songselAlloc (sequenceGetDanceList (pl->sequence),
            playlistFilterSong, pl);
      }
      danceIdx = sequenceIterate (pl->sequence);
      logMsg (LOG_DBG, LOG_BASIC, "sequence: dance: %zd", danceIdx);
      song = songselSelect (pl->songsel, danceIdx);
      count = 0;
      while (song != NULL && count < VALID_SONG_ATTEMPTS) {
        if (songAudioFileExists (song)) {
          if (checkProc (song, userdata)) {
            break;
          }
        }
        song = songselSelect (pl->songsel, danceIdx);
        ++count;
      }
      if (count >= VALID_SONG_ATTEMPTS) {
        song = NULL;
      } else {
        songselSelectFinalize (pl->songsel, danceIdx);
        sfname = songGetData (song, TAG_FILE);
        logMsg (LOG_DBG, LOG_BASIC, "sequence: select: %s", sfname);
      }
      break;
    }
  }
  return song;
}

bool
playlistFilterSong (dbidx_t dbidx, song_t *song, void *tplaylist)
{
  playlist_t    *pl = tplaylist;
  ilistidx_t    rating;
  nlistidx_t    plRating;
  nlistidx_t    plLevelLow;
  nlistidx_t    plLevelHigh;
  ilistidx_t    level;
  ilistidx_t    danceIdx;
  char          *keyword;
  nlistidx_t    idx;
  ssize_t       plbpmhigh;
  ssize_t       plbpmlow;


  plRating = nlistGetNum (pl->plinfo, PLAYLIST_RATING);
  rating = songGetNum (song, TAG_DANCERATING);
  if (rating < plRating) {
    logMsg (LOG_DBG, LOG_SONGSEL, "reject: %zd rating %ld < %ld", dbidx, rating, plRating);
    return false;
  }

  plLevelLow = nlistGetNum (pl->plinfo, PLAYLIST_LEVEL_LOW);
  plLevelHigh = nlistGetNum (pl->plinfo, PLAYLIST_LEVEL_HIGH);
  level = songGetNum (song, TAG_DANCELEVEL);
  if (level < plLevelLow || level > plLevelHigh) {
    logMsg (LOG_DBG, LOG_SONGSEL, "reject: %zd level %ld < %ld / > %ld", dbidx, level, plLevelLow, plLevelHigh);
    return false;
  }

  keyword = songGetData (song, TAG_KEYWORD);
  if (keyword != NULL) {
    slist_t        *kwList;

    kwList = nlistGetList (pl->plinfo, PLAYLIST_ALLOWED_KEYWORDS);
    idx = slistGetIdx (kwList, keyword);
    if (slistGetCount (kwList) > 0 && idx < 0) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject: %zd keyword %s not in allowed", dbidx, keyword);
      return false;
    }
  }

  if (nlistGetNum (pl->plinfo, PLAYLIST_USE_STATUS)) {
    status_t      *status;
    listidx_t     sstatus;

    sstatus = songGetNum (song, TAG_STATUS);
    status = bdjvarsdf [BDJVDF_STATUS];
    if (status != NULL && ! statusPlayCheck (status, sstatus)) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject %zd status %ld not playable", dbidx, status);
      return false;
    }
  }

  danceIdx = songGetNum (song, TAG_DANCE);
  plbpmlow = ilistGetNum (pl->pldances, danceIdx, PLDANCE_BPM_LOW);
  plbpmhigh = ilistGetNum (pl->pldances, danceIdx, PLDANCE_BPM_HIGH);
  if (plbpmlow > 0 && plbpmhigh > 0) {
    ssize_t     bpm;

    bpm = songGetNum (song, TAG_BPM);
    if (bpm < 0 || bpm < plbpmlow || bpm > plbpmhigh) {
      logMsg (LOG_DBG, LOG_SONGSEL, "reject %zd bpm %zd [%zd,%zd]", dbidx, bpm, plbpmlow, plbpmhigh);
      return false;
    }
  }

  return true;
}

slist_t *
playlistGetPlaylistList (void)
{
  char        *tplfnm;
  char        tfn [MAXPATHLEN];
  slist_t     *filelist;
  slist_t     *pnlist;
  playlist_t  *pl;
  pathinfo_t  *pi;


  pnlist = slistAlloc ("playlistlist", LIST_ORDERED, free, free);

  datautilMakePath (tfn, sizeof (tfn), "", "", "", DATAUTIL_MP_NONE);
  filelist = filemanipBasicDirList (tfn, ".pl");

  slistStartIterator (filelist);
  while ((tplfnm = slistIterateKey (filelist)) != NULL) {
    pi = pathInfo (tplfnm);
    strlcpy (tfn, pi->basename, MAXPATHLEN);
    tfn [pi->blen] = '\0';
    pl = playlistAlloc (tfn);
    if (pl != NULL) {
      slistSetData (pnlist, pl->name, strdup (tfn));
      playlistFree (pl);
    }
    pathInfoFree (pi);
  }

  slistFree (filelist);

  return pnlist;
}


/* internal routines */

static void
plConvType (char *data, datafileret_t *ret)
{
  ret->valuetype = VALUE_NUM;
  ret->u.num = PLTYPE_MANUAL;
  if (strcmp (data, "Automatic") == 0) {
    ret->u.num = PLTYPE_AUTO;
  }
  if (strcmp (data, "Sequence") == 0) {
    ret->u.num = PLTYPE_SEQ;
  }
}

void
playlistAddPlayed (playlist_t *pl, song_t *song)
{
  pltype_t      type;
  ilistidx_t     danceIdx;


  type = (pltype_t) nlistGetNum (pl->plinfo, PLAYLIST_TYPE);

  /* only the automatic playlists need to track which dances have been played */
  if (type != PLTYPE_AUTO) {
    return;
  }
  danceIdx = songGetNum (song, TAG_DANCE);
}

