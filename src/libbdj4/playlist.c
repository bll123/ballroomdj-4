#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdj4intl.h"
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

typedef struct playlist {
  char          *name;
  musicdb_t     *musicdb;
  datafile_t    *plinfodf;
  datafile_t    *pldancesdf;
  songlist_t    *songlist;
  songfilter_t  *songfilter;
  sequence_t    *sequence;
  songsel_t     *songsel;
  dancesel_t    *dancesel;
  nlist_t       *plinfo;
  ilist_t       *pldances;
  nlist_t       *countList;
  int           songlistIdx;
  int           count;
  nlistidx_t    seqiteridx;
} playlist_t;

static void plConvType (datafileconv_t *conv);

/* must be sorted in ascii order */
static datafilekey_t playlistdfkeys [PLAYLIST_KEY_MAX] = {
  { "ALLOWEDKEYWORDS",PLAYLIST_ALLOWED_KEYWORDS, VALUE_LIST, convTextList, -1 },
  { "DANCELEVELHIGH", PLAYLIST_LEVEL_HIGH, VALUE_NUM, levelConv, -1 },
  { "DANCELEVELLOW",  PLAYLIST_LEVEL_LOW, VALUE_NUM, levelConv, -1 },
  { "DANCERATING",    PLAYLIST_RATING, VALUE_NUM, ratingConv, -1 },
  { "GAP",            PLAYLIST_GAP, VALUE_NUM, NULL, -1 },
  { "MAXPLAYTIME",    PLAYLIST_MAX_PLAY_TIME, VALUE_NUM, NULL, -1 },
  { "PLAYANNOUNCE",   PLAYLIST_ANNOUNCE, VALUE_NUM, convBoolean, -1 },
  { "STOPAFTER",      PLAYLIST_STOP_AFTER, VALUE_NUM, NULL, -1 },
  { "STOPTIME",       PLAYLIST_STOP_TIME, VALUE_NUM, NULL, -1 },
  { "TYPE",           PLAYLIST_TYPE, VALUE_NUM, plConvType, -1 },
};

/* must be sorted in ascii order */
static datafilekey_t playlistdancedfkeys [PLDANCE_KEY_MAX] = {
  { "BPMHIGH",        PLDANCE_BPM_HIGH,     VALUE_NUM, NULL, -1 },
  { "BPMLOW",         PLDANCE_BPM_LOW,      VALUE_NUM, NULL, -1 },
  { "COUNT",          PLDANCE_COUNT,        VALUE_NUM, NULL, -1 },
  { "DANCE",          PLDANCE_DANCE,        VALUE_NUM, danceConvDance, -1 },
  { "MAXPLAYTIME",    PLDANCE_MAXPLAYTIME,  VALUE_NUM, NULL, -1 },
  { "SELECTED",       PLDANCE_SELECTED,     VALUE_NUM, convBoolean, -1 },
};

static void playlistSetSongFilter (playlist_t *pl);
static void playlistCountList (playlist_t *pl);

playlist_t *
playlistAlloc (musicdb_t *musicdb)
{
  playlist_t    *pl = NULL;

  pl = malloc (sizeof (playlist_t));
  assert (pl != NULL);
  pl->name = NULL;
  pl->songlistIdx = 0;
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
  pl->musicdb = musicdb;

  return pl;
}

void
playlistFree (void *tpl)
{
  playlist_t      *pl = tpl;

  if (pl != NULL) {
    if (pl->plinfodf != NULL) {
      datafileFree (pl->plinfodf);
    } else {
      if (pl->plinfo != NULL) {
        nlistFree (pl->plinfo);
      }
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

int
playlistLoad (playlist_t *pl, const char *fname)
{
  char          tfn [MAXPATHLEN];
  pltype_t      type;
  ilist_t       *tpldances;
  ilistidx_t    tidx;
  ilistidx_t    didx;
  ilistidx_t    iteridx;
  dance_t       *dances;
  nlist_t       *tlist;

  if (fname == NULL) {
    return -1;
  }

  pathbldMakePath (tfn, sizeof (tfn), fname,
      BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  if (pl == NULL) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Null playlist %s", tfn);
    return -1;
  }
  if (! fileopFileExists (tfn)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Missing playlist-pl %s", tfn);
    return -1;
  }

  pl->name = strdup (fname);
  pl->plinfodf = datafileAllocParse ("playlist-pl", DFTYPE_KEY_VAL, tfn,
      playlistdfkeys, PLAYLIST_KEY_MAX, DATAFILE_NO_LOOKUP);
  if (! fileopFileExists (tfn)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Bad playlist-pl %s", tfn);
    playlistFree (pl);
    return -1;
  }
  pl->plinfo = datafileGetList (pl->plinfodf);
  nlistDumpInfo (pl->plinfo);

  pathbldMakePath (tfn, sizeof (tfn), fname,
      BDJ4_PL_DANCE_EXT, PATHBLD_MP_DATA);
  if (! fileopFileExists (tfn)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Missing playlist-dance %s", tfn);
    playlistFree (pl);
    return -1;
  }

  pl->pldancesdf = datafileAllocParse ("playlist-dances", DFTYPE_INDIRECT, tfn,
      playlistdancedfkeys, PLDANCE_KEY_MAX, DATAFILE_NO_LOOKUP);
  if (pl->pldancesdf == NULL) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Bad playlist-dance %s", tfn);
    playlistFree (pl);
    return -1;
  }
  tpldances = datafileGetList (pl->pldancesdf);

  /* pldances must be rebuilt to use the dance key as the index   */
  /* the playlist datafiles have a generic key value */
  logMsg (LOG_DBG, LOG_IMPORTANT, "rebuilding playlist");
  pl->pldances = ilistAlloc ("playlist-dances-n", LIST_ORDERED);
  ilistSetSize (pl->pldances, ilistGetCount (tpldances));
  ilistStartIterator (tpldances, &iteridx);
  while ((tidx = ilistIterateKey (tpldances, &iteridx)) >= 0) {
    /* have to make a clone of the data */
    didx = ilistGetNum (tpldances, tidx, PLDANCE_DANCE);
    for (size_t i = 0; i < PLDANCE_KEY_MAX; ++i) {
      ilistSetNum (pl->pldances, didx, playlistdancedfkeys [i].itemkey,
          ilistGetNum (tpldances, tidx, playlistdancedfkeys [i].itemkey));
    }
  }
  datafileFree (pl->pldancesdf);
  pl->pldancesdf = NULL;

  ilistDumpInfo (pl->pldances);

  type = (pltype_t) nlistGetNum (pl->plinfo, PLAYLIST_TYPE);

  if (type == PLTYPE_SONGLIST) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "songlist: load songlist %s", fname);
    pathbldMakePath (tfn, sizeof (tfn), fname,
        BDJ4_SONGLIST_EXT, PATHBLD_MP_DATA);
    if (! fileopFileExists (tfn)) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Missing songlist %s", tfn);
      playlistFree (pl);
      return -1;
    }
    pl->songlist = songlistAlloc (tfn);
    if (pl->songlist == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Bad songlist %s", tfn);
      playlistFree (pl);
      return -1;
    }
  }

  if (type == PLTYPE_SEQUENCE) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "sequence: load sequence %s", fname);
    pathbldMakePath (tfn, sizeof (tfn), fname,
        BDJ4_SEQUENCE_EXT, PATHBLD_MP_DATA);
    if (! fileopFileExists (tfn)) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Missing sequence %s", tfn);
      playlistFree (pl);
      return -1;
    }
    pl->sequence = sequenceAlloc (fname);
    if (pl->sequence == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Bad sequence %s", fname);
      playlistFree (pl);
      return -1;
    }

    /* reset and set all of the 'selected' flags. */
    /* this just makes playlist management work better for sequences */
    dances = bdjvarsdfGet (BDJVDF_DANCES);
    danceStartIterator (dances, &iteridx);
    while ((didx = danceIterate (dances, &iteridx)) >= 0) {
      ilistSetNum (pl->pldances, didx, PLDANCE_SELECTED, 0);
    }
    /* the sequence iterator doesn't stop, use the returned dance list */
    tlist = sequenceGetDanceList (pl->sequence);
    nlistStartIterator (tlist, &iteridx);
    while ((didx = nlistIterateKey (tlist, &iteridx)) >= 0) {
      ilistSetNum (pl->pldances, didx, PLDANCE_SELECTED, 1);
    }

    sequenceStartIterator (pl->sequence, &pl->seqiteridx);
  }

  return 0;
}

void
playlistCreate (playlist_t *pl, const char *plfname, pltype_t type,
    const char *suppfname)
{
  char          tbuff [40];
  ilistidx_t    didx;
  dance_t       *dances;
  level_t       *levels;
  ilistidx_t    iteridx;


  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  pl->name = strdup (plfname);
  snprintf (tbuff, sizeof (tbuff), "plinfo-c-%s", plfname);
  pl->plinfo = nlistAlloc (tbuff, LIST_UNORDERED, free);
  nlistSetSize (pl->plinfo, PLAYLIST_KEY_MAX);
  nlistSetStr (pl->plinfo, PLAYLIST_ALLOWED_KEYWORDS, NULL);
  nlistSetNum (pl->plinfo, PLAYLIST_ANNOUNCE, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_GAP, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_LEVEL_HIGH, levelGetMax (levels));
  nlistSetNum (pl->plinfo, PLAYLIST_LEVEL_LOW, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_MAX_PLAY_TIME, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_RATING, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_TYPE, type);
  nlistSetNum (pl->plinfo, PLAYLIST_STOP_AFTER, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_STOP_TIME, LIST_VALUE_INVALID);
  nlistSort (pl->plinfo);

  if (suppfname == NULL) {
    suppfname = plfname;
  }
  if (type == PLTYPE_SONGLIST) {
    pl->songlist = songlistAlloc (suppfname);
  }
  if (type == PLTYPE_SEQUENCE) {
    pl->sequence = sequenceAlloc (suppfname);
  }

  snprintf (tbuff, sizeof (tbuff), "pldance-c-%s", plfname);
  pl->pldances = ilistAlloc (tbuff, LIST_ORDERED);

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceStartIterator (dances, &iteridx);
  while ((didx = danceIterate (dances, &iteridx)) >= 0) {
    ilistSetNum (pl->pldances, didx, PLDANCE_BPM_HIGH, LIST_VALUE_INVALID);
    ilistSetNum (pl->pldances, didx, PLDANCE_BPM_LOW, LIST_VALUE_INVALID);
    ilistSetNum (pl->pldances, didx, PLDANCE_COUNT, 0);
    ilistSetNum (pl->pldances, didx, PLDANCE_DANCE, didx);
    ilistSetNum (pl->pldances, didx, PLDANCE_MAXPLAYTIME, 0);
    ilistSetNum (pl->pldances, didx, PLDANCE_SELECTED, 0);
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

void
playlistSetConfigList (playlist_t *pl, playlistkey_t key, const char *value)
{
  datafileconv_t  conv;

  if (pl == NULL || pl->plinfo == NULL) {
    return;
  }

  conv.str = strdup (value);
  conv.allocated = true;
  conv.valuetype = VALUE_STR;
  convTextList (&conv);

  nlistSetList (pl->plinfo, key, conv.list);
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

  ilistSetNum (pl->pldances, danceIdx, PLDANCE_COUNT, count);
  if (count > 0) {
    ilistSetNum (pl->pldances, danceIdx, PLDANCE_SELECTED, 1);
  } else {
    ilistSetNum (pl->pldances, danceIdx, PLDANCE_SELECTED, 0);
  }
  return;
}

void
playlistSetDanceNum (playlist_t *pl, ilistidx_t danceIdx, pldancekey_t key, ssize_t value)
{
  if (pl == NULL || pl->plinfo == NULL) {
    return;
  }

  ilistSetNum (pl->pldances, danceIdx, key, value);
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

  if (type == PLTYPE_AUTO || type == PLTYPE_SEQUENCE) {
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
        pl->songsel = songselAlloc (pl->musicdb,
            pl->countList, pl->songfilter);
      }
    }
    if (type == PLTYPE_SEQUENCE) {
      if (pl->songsel == NULL) {
        playlistSetSongFilter (pl);
        pl->songsel = songselAlloc (pl->musicdb,
            sequenceGetDanceList (pl->sequence), pl->songfilter);
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
      sfname = songGetStr (song, TAG_FILE);
      ++pl->count;
      logMsg (LOG_DBG, LOG_BASIC, "sequence: select: %s", sfname);
    }
  }
  if (type == PLTYPE_SONGLIST) {
    sfname = songlistGetNext (pl->songlist, pl->songlistIdx, SONGLIST_FILE);
    while (sfname != NULL) {
      song = dbGetByName (pl->musicdb, sfname);
      if (song != NULL && songAudioFileExists (song)) {
        ilistidx_t  tval;
        char        *tstr;

        /* check for unknown dances */
        /* put something into the marquee display field if possible */
        tval = songGetNum (song, TAG_DANCE);
        if (tval < 0) {
          tstr = songGetStr (song, TAG_MQDISPLAY);
          if (tstr == NULL) {
            tstr = songlistGetNext (pl->songlist, pl->songlistIdx, SONGLIST_DANCESTR);
            if (tstr != NULL) {
              songSetStr (song, TAG_MQDISPLAY, tstr);
            }
          }
        }
        break;
      }
      song = NULL;
      logMsg (LOG_DBG, LOG_IMPORTANT, "songlist: missing: %s", sfname);
      ++pl->songlistIdx;
      sfname = songlistGetNext (pl->songlist, pl->songlistIdx, SONGLIST_FILE);
    }
    ++pl->songlistIdx;
    ++pl->count;
    logMsg (LOG_DBG, LOG_BASIC, "songlist: select: %s", sfname);
  }
  logProcEnd (LOG_PROC, "playlistGetNextSong", "");
  return song;
}

slist_t *
playlistGetPlaylistList (int flag)
{
  char        *tplfnm;
  char        tfn [MAXPATHLEN];
  slist_t     *filelist;
  slist_t     *pnlist;
  pathinfo_t  *pi;
  slistidx_t  iteridx;
  char        *ext = NULL;


  pnlist = slistAlloc ("playlistlist", LIST_ORDERED, free);

  pathbldMakePath (tfn, sizeof (tfn), "", "", PATHBLD_MP_DATA);
  ext = BDJ4_PLAYLIST_EXT;
  if (flag == PL_LIST_SONGLIST) {
    ext = BDJ4_SONGLIST_EXT;
  }
  if (flag == PL_LIST_SEQUENCE) {
    ext = BDJ4_SEQUENCE_EXT;
  }
  if (flag == PL_LIST_ALL) {
    ext = BDJ4_PLAYLIST_EXT;
  }
  filelist = filemanipBasicDirList (tfn, ext);

  slistStartIterator (filelist, &iteridx);
  while ((tplfnm = slistIterateKey (filelist, &iteridx)) != NULL) {
    pi = pathInfo (tplfnm);
    strlcpy (tfn, pi->basename, pi->blen + 1);
    tfn [pi->blen] = '\0';

    pathInfoFree (pi);
    if (flag == PL_LIST_NORMAL &&
        /* CONTEXT: the name for the special playlist used for the 'queue dance' button */
        strcmp (tfn, _("QueueDance")) == 0) {
      continue;
    }
    slistSetStr (pnlist, tfn, tfn);
  }

  slistFree (filelist);

  return pnlist;
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

void
playlistSave (playlist_t *pl, const char *name)
{
  char  tfn [MAXPATHLEN];

  if (name != NULL) {
    if (pl->name != NULL) {
      free (pl->name);
    }
    pl->name = strdup (name);
  }

  pathbldMakePath (tfn, sizeof (tfn), pl->name,
      BDJ4_PLAYLIST_EXT, PATHBLD_MP_DATA);
  datafileSaveKeyVal ("playlist", tfn, playlistdfkeys,
      PLAYLIST_KEY_MAX, pl->plinfo);

  pathbldMakePath (tfn, sizeof (tfn), pl->name,
      BDJ4_PL_DANCE_EXT, PATHBLD_MP_DATA);
  datafileSaveIndirect ("playlist", tfn, playlistdancedfkeys,
      PLDANCE_KEY_MAX, pl->pldances);
}

/* internal routines */

static void
plConvType (datafileconv_t *conv)
{
  conv->allocated = false;
  if (conv->valuetype == VALUE_STR) {
    ssize_t   num;

    conv->valuetype = VALUE_NUM;
    num = PLTYPE_SONGLIST;
    if (strcmp (conv->str, "automatic") == 0) {
      num = PLTYPE_AUTO;
    }
    if (strcmp (conv->str, "sequence") == 0) {
      num = PLTYPE_SEQUENCE;
    }
    conv->num = num;
  } else if (conv->valuetype == VALUE_NUM) {
    char    *sval;

    conv->valuetype = VALUE_STR;
    sval = "songlist";
    switch (conv->num) {
      case PLTYPE_SONGLIST: { sval = "songlist"; break; }
      case PLTYPE_AUTO: { sval = "automatic"; break; }
      case PLTYPE_SEQUENCE: { sval = "sequence"; break; }
    }
    conv->str = sval;
  }
}

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
  pl->songfilter = songfilterAlloc ();
  assert (pl->songfilter != NULL);
  songfilterSetNum (pl->songfilter, SONG_FILTER_STATUS_PLAYABLE,
      SONG_FILTER_FOR_PLAYBACK);

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

  danceList = ilistAlloc ("pl-dance-filter", LIST_ORDERED);
  ilistStartIterator (pl->pldances, &iteridx);
  while ((danceIdx = ilistIterateKey (pl->pldances, &iteridx)) >= 0) {
    ssize_t       sel;

    sel = ilistGetNum (pl->pldances, danceIdx, PLDANCE_SELECTED);
    if (sel == 1) {
      /* any value will work; the danceIdx just needs to exist in the list */
      ilistSetNum (danceList, danceIdx, 0, 0);

      plbpmlow = ilistGetNum (pl->pldances, danceIdx, PLDANCE_BPM_LOW);
      plbpmhigh = ilistGetNum (pl->pldances, danceIdx, PLDANCE_BPM_HIGH);
      if (plbpmlow > 0 && plbpmhigh > 0) {
        songfilterDanceSet (pl->songfilter, danceIdx, SONG_FILTER_BPM_LOW, plbpmlow);
        songfilterDanceSet (pl->songfilter, danceIdx, SONG_FILTER_BPM_HIGH, plbpmhigh);
      }
    }
  }

  songfilterSetData (pl->songfilter, SONG_FILTER_DANCE, danceList);
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
