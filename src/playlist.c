#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "datafile.h"
#include "pathbld.h"
#include "filemanip.h"
#include "fileop.h"
#include "ilist.h"
#include "level.h"
#include "log.h"
#include "nlist.h"
#include "musicdb.h"
#include "pathutil.h"
#include "playlist.h"
#include "rating.h"
#include "sequence.h"
#include "slist.h"
#include "song.h"
#include "songfilter.h"
#include "songlist.h"
#include "songsel.h"
#include "status.h"
#include "tagdef.h"

static void     plConvType (char *, datafileret_t *ret);

/* must be sorted in ascii order */
static datafilekey_t playlistdfkeys[] = {
  { "ALLOWEDKEYWORDS",  PLAYLIST_ALLOWED_KEYWORDS,
      VALUE_DATA, parseConvTextList, -1 },
  { "DANCELEVELHIGH",   PLAYLIST_LEVEL_HIGH,
      VALUE_NUM, levelConv, -1 },
  { "DANCELEVELLOW",    PLAYLIST_LEVEL_LOW,
      VALUE_NUM, levelConv, -1 },
  { "DANCERATING",      PLAYLIST_RATING,
      VALUE_NUM, ratingConv, -1 },
  { "GAP",              PLAYLIST_GAP,
      VALUE_NUM, NULL, -1 },
  { "MANUALLIST",       PLAYLIST_MANUAL_LIST_NAME,
      VALUE_DATA, NULL, -1 },
  { "MAXPLAYTIME",      PLAYLIST_MAX_PLAY_TIME,
      VALUE_NUM, NULL, -1 },
  { "MQMESSAGE",        PLAYLIST_MQ_MESSAGE,
      VALUE_DATA, NULL, -1 },
  { "PAUSEEACHSONG",    PLAYLIST_PAUSE_EACH_SONG,
      VALUE_NUM, parseConvBoolean, -1 },
  { "PLAYANNOUNCE",     PLAYLIST_ANNOUNCE,
      VALUE_NUM, parseConvBoolean, -1 },
  { "SEQUENCE",         PLAYLIST_SEQ_NAME,
      VALUE_DATA, NULL, -1 },
  { "STOPAFTER",        PLAYLIST_STOP_AFTER,
      VALUE_NUM, NULL, -1 },
  { "STOPTIME",         PLAYLIST_STOP_TIME,
      VALUE_NUM, NULL, -1 },
  { "TYPE",             PLAYLIST_TYPE,
      VALUE_NUM, plConvType, -1 },
};
#define PLAYLIST_DFKEY_COUNT (sizeof (playlistdfkeys) / sizeof (datafilekey_t))

/* must be sorted in ascii order */
static datafilekey_t playlistdancedfkeys[] = {
  { "BPMHIGH",        PLDANCE_BPM_HIGH,     VALUE_NUM, NULL, -1 },
  { "BPMLOW",         PLDANCE_BPM_LOW,      VALUE_NUM, NULL, -1 },
  { "COUNT",          PLDANCE_COUNT,        VALUE_NUM, NULL, -1 },
  { "DANCE",          PLDANCE_DANCE,        VALUE_DATA, danceConvDance, -1 },
  { "MAXPLAYTIME",    PLDANCE_MAXPLAYTIME,  VALUE_NUM, NULL, -1 },
  { "SELECTED",       PLDANCE_SELECTED,     VALUE_NUM, parseConvBoolean, -1 },
};
#define PLAYLIST_DANCE_DFKEY_COUNT (sizeof (playlistdancedfkeys) / sizeof (datafilekey_t))

static void playlistSetSongFilter (playlist_t *pl);
static void playlistCountList (playlist_t *pl);

playlist_t *
playlistLoad (char *fname)
{
  playlist_t    *pl = NULL;
  char          tfn [MAXPATHLEN];
  pltype_t      type;
  ilist_t       *tpldances;
  ilistidx_t    tidx;
  ilistidx_t    didx;
  ilistidx_t    iteridx;


  pathbldMakePath (tfn, sizeof (tfn), "", fname, ".pl", PATHBLD_MP_NONE);
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

  pathbldMakePath (tfn, sizeof (tfn), "", fname, ".pldances", PATHBLD_MP_NONE);
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
  tpldances = datafileGetList (pl->pldancesdf);

  if (ilistGetVersion (tpldances) < 2) {
    /* pldances must be rebuilt to use the dance key as the index   */
    /* all of the old playlists have a generic key value            */
    pl->pldances = ilistAlloc ("playlist-dances-n", LIST_ORDERED, nlistFree);
    ilistSetSize (pl->pldances, ilistGetCount (tpldances));
    ilistStartIterator (tpldances, &iteridx);
    while ((tidx = ilistIterateKey (tpldances, &iteridx)) >= 0) {
      /* have to make a clone of the data */
      didx = ilistGetNum (tpldances, tidx, PLDANCE_DANCE);
      for (size_t i = 0; i < PLAYLIST_DANCE_DFKEY_COUNT; ++i) {
        ilistSetNum (pl->pldances, didx, playlistdancedfkeys [i].itemkey,
            ilistGetNum (tpldances, tidx, playlistdancedfkeys [i].itemkey));
      }
    }
    datafileFree (pl->pldancesdf);
    pl->pldancesdf = NULL;
  } else {
    pl->pldances = tpldances;
  }

  ilistDumpInfo (pl->pldances);

  type = (pltype_t) nlistGetNum (pl->plinfo, PLAYLIST_TYPE);

  if (type == PLTYPE_MANUAL) {
    char *slfname = nlistGetStr (pl->plinfo, PLAYLIST_MANUAL_LIST_NAME);
    pathbldMakePath (tfn, sizeof (tfn), "", slfname, ".songlist", PATHBLD_MP_NONE);
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
    pathbldMakePath (tfn, sizeof (tfn), "", seqfname, ".sequence", PATHBLD_MP_NONE);
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
    sequenceStartIterator (pl->sequence, &pl->seqiteridx);
  }

  return pl;
}

playlist_t *
playlistCreate (char *plfname, pltype_t type, char *ofname)
{
  playlist_t    *pl = NULL;
  char          tbuff [40];
  ilistidx_t    didx;
  dance_t       *dances;
  level_t       *levels;
  ilistidx_t    iteridx;


  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  pl = playlistAlloc (plfname);

  snprintf (tbuff, sizeof (tbuff), "plinfo-c-%s", plfname);
  pl->plinfo = nlistAlloc (tbuff, LIST_UNORDERED, free);
  nlistSetSize (pl->plinfo, PLAYLIST_KEY_MAX);
  nlistSetStr (pl->plinfo, PLAYLIST_ALLOWED_KEYWORDS, NULL);
  nlistSetNum (pl->plinfo, PLAYLIST_ANNOUNCE, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_GAP, 1000);
  nlistSetNum (pl->plinfo, PLAYLIST_LEVEL_HIGH, levelGetMax (levels));
  nlistSetNum (pl->plinfo, PLAYLIST_LEVEL_LOW, 0);
  nlistSetStr (pl->plinfo, PLAYLIST_MANUAL_LIST_NAME, NULL);
  nlistSetNum (pl->plinfo, PLAYLIST_MAX_PLAY_TIME, 0);
  nlistSetStr (pl->plinfo, PLAYLIST_MQ_MESSAGE, NULL);
  nlistSetNum (pl->plinfo, PLAYLIST_PAUSE_EACH_SONG, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_RATING, 0);
  nlistSetStr (pl->plinfo, PLAYLIST_SEQ_NAME, NULL);
  nlistSetNum (pl->plinfo, PLAYLIST_TYPE, type);
  nlistSetNum (pl->plinfo, PLAYLIST_STOP_AFTER, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_STOP_TIME, LIST_VALUE_INVALID);
  nlistSort (pl->plinfo);

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

  snprintf (tbuff, sizeof (tbuff), "pldance-c-%s", plfname);
  pl->pldances = ilistAlloc (tbuff, LIST_ORDERED, NULL);

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceStartIterator (dances, &iteridx);
  while ((didx = danceIterateKey (dances, &iteridx)) >= 0) {
    ilistSetNum (pl->pldances, didx, PLDANCE_BPM_HIGH, LIST_VALUE_INVALID);
    ilistSetNum (pl->pldances, didx, PLDANCE_BPM_LOW, LIST_VALUE_INVALID);
    ilistSetNum (pl->pldances, didx, PLDANCE_COUNT, 0);
    ilistSetNum (pl->pldances, didx, PLDANCE_DANCE, didx);
    ilistSetNum (pl->pldances, didx, PLDANCE_MAXPLAYTIME, 0);
    ilistSetNum (pl->pldances, didx, PLDANCE_SELECTED, 0);
  }

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
  pl->songfilter = NULL;
  pl->sequence = NULL;
  pl->songsel = NULL;
  pl->dancesel = NULL;
  pl->plinfo = NULL;
  pl->pldances = NULL;
  pl->countList = NULL;
  pl->count = 0;

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
    } else {
      if (pl->pldances != NULL) {
        ilistFree (pl->pldances);
      }
    }
    if (pl->songlist != NULL) {
      songlistFree (pl->songlist);
    }
    if (pl->songfilter != NULL) {
      songfilterFree (pl->songfilter);
    }
    if (pl->sequence != NULL) {
      sequenceFree (pl->sequence);
    }
    if (pl->songsel != NULL) {
      songselFree (pl->songsel);
    }
    if (pl->dancesel != NULL) {
      danceselFree (pl->dancesel);
    }
    if (pl->countList != NULL) {
      nlistFree (pl->countList);
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

void
playlistSetConfigNum (playlist_t *pl, playlistkey_t key, ssize_t value)
{
  if (pl == NULL || pl->plinfo == NULL) {
    return;
  }

  nlistSetNum (pl->plinfo, key, value);
  return;
}

ssize_t
playlistGetDanceNum (playlist_t *pl, ilistidx_t danceIdx, pldancekey_t key)
{
  ssize_t     val;

  if (pl == NULL || pl->plinfo == NULL) {
    return LIST_VALUE_INVALID;
  }

  val = ilistGetNum (pl->pldances, danceIdx, key);
  return val;
}

void
playlistSetDanceCount (playlist_t *pl, ilistidx_t danceIdx, ssize_t count)
{
  if (pl == NULL || pl->plinfo == NULL) {
    return;
  }

  ilistSetNum (pl->pldances, danceIdx, PLDANCE_SELECTED, 1);
  ilistSetNum (pl->pldances, danceIdx, PLDANCE_COUNT, count);
  return;
}

song_t *
playlistGetNextSong (playlist_t *pl, nlist_t *danceCounts,
    ssize_t priorCount, playlistCheck_t checkProc,
    danceselHistory_t historyProc, void *userdata)
{
  pltype_t    type;
  song_t      *song = NULL;
  int         count;
  char        *sfname;
  ssize_t     stopAfter;


  if (pl == NULL) {
    return NULL;
  }

  logProcBegin (LOG_PROC, "playlistGetNextSong");
  type = (pltype_t) nlistGetNum (pl->plinfo, PLAYLIST_TYPE);
  stopAfter = nlistGetNum (pl->plinfo, PLAYLIST_STOP_AFTER);
  if (stopAfter > 0 && pl->count >= stopAfter) {
    logMsg (LOG_DBG, LOG_BASIC, "pl %s stop after %ld", pl->name, stopAfter);
    return NULL;
  }

  if (type == PLTYPE_AUTO || type == PLTYPE_SEQ) {
    ilistidx_t     danceIdx = LIST_VALUE_INVALID;

    if (type == PLTYPE_AUTO) {
      if (pl->countList == NULL) {
        playlistCountList (pl);
      }
      if (pl->dancesel == NULL) {
        pl->dancesel = danceselAlloc (pl->countList);
      }
      danceIdx = danceselSelect (pl->dancesel, danceCounts,
          priorCount, historyProc, userdata);
      logMsg (LOG_DBG, LOG_BASIC, "automatic: dance: %zd", danceIdx);
      if (pl->songsel == NULL) {
        playlistSetSongFilter (pl);
        pl->songsel = songselAlloc (pl->countList, pl->songfilter);
      }
    }
    if (type == PLTYPE_SEQ) {
      if (pl->songsel == NULL) {
        playlistSetSongFilter (pl);
        pl->songsel = songselAlloc (sequenceGetDanceList (pl->sequence),
            pl->songfilter);
      }
      danceIdx = sequenceIterate (pl->sequence, &pl->seqiteridx);
      logMsg (LOG_DBG, LOG_BASIC, "sequence: dance: %zd", danceIdx);
    }

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
      ++pl->count;
      logMsg (LOG_DBG, LOG_BASIC, "sequence: select: %s", sfname);
    }
  }
  if (type == PLTYPE_MANUAL) {
    sfname = songlistGetData (pl->songlist, pl->manualIdx, SONGLIST_FILE);
    while (sfname != NULL) {
      song = dbGetByName (sfname);
      if (song != NULL && songAudioFileExists (song)) {
        ilistidx_t  tval;
        char        *tstr;

        /* check for unknown dances */
        /* put something into the marquee display field if possible */
        tval = songGetNum (song, TAG_DANCE);
        if (tval < 0) {
          tstr = songGetData (song, TAG_MQDISPLAY);
          if (tstr == NULL) {
            tstr = songlistGetData (pl->songlist, pl->manualIdx, SONGLIST_DANCESTR);
            if (tstr != NULL) {
              songSetData (song, TAG_MQDISPLAY, strdup (tstr));
            }
          }
        }
        break;
      }
      song = NULL;
      logMsg (LOG_DBG, LOG_IMPORTANT, "manual: missing: %s", sfname);
      ++pl->manualIdx;
      sfname = songlistGetData (pl->songlist, pl->manualIdx, SONGLIST_FILE);
    }
    ++pl->manualIdx;
    ++pl->count;
    logMsg (LOG_DBG, LOG_BASIC, "manual: select: %s", sfname);
  }
  logProcEnd (LOG_PROC, "playlistGetNextSong", "");
  return song;
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
  slistidx_t  iteridx;


  pnlist = slistAlloc ("playlistlist", LIST_ORDERED, free, free);

  pathbldMakePath (tfn, sizeof (tfn), "", "", "", PATHBLD_MP_NONE);
  filelist = filemanipBasicDirList (tfn, ".pl");

  slistStartIterator (filelist, &iteridx);
  while ((tplfnm = slistIterateKey (filelist, &iteridx)) != NULL) {
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
playlistSetSongFilter (playlist_t *pl)
{
  nlistidx_t    plRating;
  nlistidx_t    plLevelLow;
  nlistidx_t    plLevelHigh;
  ilistidx_t    danceIdx;
  slist_t       *kwList;
  ssize_t       plbpmhigh;
  ssize_t       plbpmlow;
  ilist_t       *danceList;
  ilistidx_t    iteridx;



  logMsg (LOG_DBG, LOG_SONGSEL, "initializing song filter");
  pl->songfilter = songfilterAlloc (SONG_FILTER_FOR_PLAYBACK);
  assert (pl->songfilter != NULL);

  plRating = nlistGetNum (pl->plinfo, PLAYLIST_RATING);
  if (plRating != LIST_VALUE_INVALID) {
    songfilterSetNum (pl->songfilter, SONG_FILTER_RATING, plRating);
  }

  plLevelLow = nlistGetNum (pl->plinfo, PLAYLIST_LEVEL_LOW);
  plLevelHigh = nlistGetNum (pl->plinfo, PLAYLIST_LEVEL_HIGH);
  if (plLevelLow != LIST_VALUE_INVALID && plLevelHigh != LIST_VALUE_INVALID) {
    songfilterSetNum (pl->songfilter, SONG_FILTER_LEVEL_LOW, plLevelLow);
    songfilterSetNum (pl->songfilter, SONG_FILTER_LEVEL_HIGH, plLevelHigh);
  }

  kwList = nlistGetList (pl->plinfo, PLAYLIST_ALLOWED_KEYWORDS);
  if (slistGetCount (kwList) > 0) {
    songfilterSetData (pl->songfilter, SONG_FILTER_KEYWORD, kwList);
  }

  danceList = ilistAlloc ("pl-dance-filter", LIST_ORDERED, NULL);
  ilistStartIterator (pl->pldances, &iteridx);
  while ((danceIdx = ilistIterateKey (pl->pldances, &iteridx)) >= 0) {
    ssize_t       sel;

    sel = ilistGetNum (pl->pldances, danceIdx, PLDANCE_SELECTED);
    if (sel == 1) {
      /* any value will work; the danceIdx just needs to exist in the list */
      ilistSetNum (danceList, danceIdx, SONG_FILTER_BPM_LOW, 0);

      plbpmlow = ilistGetNum (pl->pldances, danceIdx, PLDANCE_BPM_LOW);
      plbpmhigh = ilistGetNum (pl->pldances, danceIdx, PLDANCE_BPM_HIGH);
      if (plbpmlow > 0 && plbpmhigh > 0) {
        songfilterDanceSet (pl->songfilter, danceIdx, SONG_FILTER_BPM_LOW, plbpmlow);
        songfilterDanceSet (pl->songfilter, danceIdx, SONG_FILTER_BPM_HIGH, plbpmhigh);
      }
    }
  }
  songfilterSetData (pl->songfilter, SONG_FILTER_DANCE, danceList);

  /* and loop again to do the BPM lookups */
  ilistStartIterator (pl->pldances, &iteridx);
  while ((danceIdx = ilistIterateKey (pl->pldances, &iteridx)) >= 0) {
    ssize_t       sel;

    sel = ilistGetNum (pl->pldances, danceIdx, PLDANCE_SELECTED);
    if (sel == 1) {
      plbpmlow = ilistGetNum (pl->pldances, danceIdx, PLDANCE_BPM_LOW);
      plbpmhigh = ilistGetNum (pl->pldances, danceIdx, PLDANCE_BPM_HIGH);
      if (plbpmlow > 0 && plbpmhigh > 0) {
        songfilterDanceSet (pl->songfilter, danceIdx, SONG_FILTER_BPM_LOW, plbpmlow);
        songfilterDanceSet (pl->songfilter, danceIdx, SONG_FILTER_BPM_HIGH, plbpmhigh);
      }
    }
  }
}

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
playlistAddCount (playlist_t *pl, song_t *song)
{
  pltype_t      type;
  ilistidx_t     danceIdx;


  type = (pltype_t) nlistGetNum (pl->plinfo, PLAYLIST_TYPE);

  /* only the automatic playlists need to track which dances have been played */
  if (type != PLTYPE_AUTO) {
    return;
  }

  logProcBegin (LOG_PROC, "playlistAddCount");
  danceIdx = songGetNum (song, TAG_DANCE);
  if (pl->dancesel != NULL) {
    danceselAddCount (pl->dancesel, danceIdx);
  }
  logProcEnd (LOG_PROC, "playAddCount", "");
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

  logProcBegin (LOG_PROC, "playlistAddPlayed");
  danceIdx = songGetNum (song, TAG_DANCE);
  if (pl->dancesel != NULL) {
    danceselAddPlayed (pl->dancesel, danceIdx);
  }
  logProcEnd (LOG_PROC, "playAddPlayed", "");
}

static void
playlistCountList (playlist_t *pl)
{
  ssize_t     sel;
  ssize_t     count;
  ilistidx_t  didx;
  ilistidx_t  iteridx;

  logProcBegin (LOG_PROC, "playlistCountList");
  if (pl->countList != NULL) {
    return;
  }

  pl->countList = nlistAlloc ("pl-countlist", LIST_ORDERED, NULL);
  ilistStartIterator (pl->pldances, &iteridx);
  while ((didx = ilistIterateKey (pl->pldances, &iteridx)) >= 0) {
    sel = ilistGetNum (pl->pldances, didx, PLDANCE_SELECTED);
    if (sel == 1) {
      count = ilistGetNum (pl->pldances, didx, PLDANCE_COUNT);
      nlistSetDouble (pl->countList, didx, (double) count);
    }
  }
  logProcEnd (LOG_PROC, "playlistCountList", "");
}
