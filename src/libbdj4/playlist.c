/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "dancesel.h"
#include "datafile.h"
#include "dirlist.h"
#include "pathbld.h"
#include "filemanip.h"
#include "fileop.h"
#include "ilist.h"
#include "level.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "musicdb.h"
#include "pathbld.h"
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
  dance_t       *dances;
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
  ilistidx_t    songlistiter;
  int           editmode;
  int           count;
  nlistidx_t    seqiteridx;
} playlist_t;

static playlist_t *playlistAlloc (musicdb_t *musicdb);
static void plConvType (datafileconv_t *conv);

/* must be sorted in ascii order */
static datafilekey_t playlistdfkeys [PLAYLIST_KEY_MAX] = {
  { "ALLOWEDKEYWORDS",PLAYLIST_ALLOWED_KEYWORDS,  VALUE_LIST, convTextList, -1 },
  { "DANCELEVELHIGH", PLAYLIST_LEVEL_HIGH,        VALUE_NUM, levelConv, -1 },
  { "DANCELEVELLOW",  PLAYLIST_LEVEL_LOW,         VALUE_NUM, levelConv, -1 },
  { "DANCERATING",    PLAYLIST_RATING,            VALUE_NUM, ratingConv, -1 },
  { "GAP",            PLAYLIST_GAP,               VALUE_NUM, NULL, -1 },
  { "MAXPLAYTIME",    PLAYLIST_MAX_PLAY_TIME,     VALUE_NUM, NULL, -1 },
  { "PLAYANNOUNCE",   PLAYLIST_ANNOUNCE,          VALUE_NUM, convBoolean, -1 },
  { "STOPAFTER",      PLAYLIST_STOP_AFTER,        VALUE_NUM, NULL, -1 },
  { "STOPTIME",       PLAYLIST_STOP_TIME,         VALUE_NUM, NULL, -1 },
  { "TYPE",           PLAYLIST_TYPE,              VALUE_NUM, plConvType, -1 },
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

void
playlistFree (void *tpl)
{
  playlist_t    *pl = tpl;

  if (pl != NULL) {
    if (pl->plinfodf != NULL) {
      datafileFree (pl->plinfodf);
    } else {
      nlistFree (pl->plinfo);
    }
    if (pl->pldancesdf != NULL) {
      datafileFree (pl->pldancesdf);
    } else {
      ilistFree (pl->pldances);
    }
    songlistFree (pl->songlist);
    songfilterFree (pl->songfilter);
    sequenceFree (pl->sequence);
    songselFree (pl->songsel);
    danceselFree (pl->dancesel);
    nlistFree (pl->countList);
    dataFree (pl->name);
    pl->name = NULL;
    mdfree (pl);
  }
}

playlist_t *
playlistLoad (const char *fname, musicdb_t *musicdb)
{
  playlist_t    *pl;
  char          tfn [MAXPATHLEN];
  pltype_t      type;
  ilist_t       *tpldances;
  ilistidx_t    tidx;
  ilistidx_t    didx;
  ilistidx_t    iteridx;
  nlist_t       *tlist;

  if (fname == NULL) {
    return NULL;
  }

  pathbldMakePath (tfn, sizeof (tfn), fname,
      BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (tfn)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: Missing playlist-pl %s", tfn);
    return NULL;
  }

  pl = playlistAlloc (musicdb);
  pl->name = mdstrdup (fname);

  pl->plinfodf = datafileAllocParse ("playlist-pl", DFTYPE_KEY_VAL, tfn,
      playlistdfkeys, PLAYLIST_KEY_MAX);
  pl->plinfo = datafileGetList (pl->plinfodf);
  nlistDumpInfo (pl->plinfo);

  pathbldMakePath (tfn, sizeof (tfn), fname,
      BDJ4_PL_DANCE_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (tfn)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: Missing playlist-dance %s", tfn);
    playlistFree (pl);
    return NULL;
  }

  pl->pldancesdf = datafileAllocParse ("playlist-dances", DFTYPE_INDIRECT, tfn,
      playlistdancedfkeys, PLDANCE_KEY_MAX);
  if (pl->pldancesdf == NULL) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: Bad playlist-dance %s", tfn);
    playlistFree (pl);
    return NULL;
  }
  tpldances = datafileGetList (pl->pldancesdf);

  /* pldances must be rebuilt to use the dance key as the index   */
  /* the playlist datafiles have a generic key value */
  logMsg (LOG_DBG, LOG_MAIN, "rebuilding playlist");
  pl->pldances = ilistAlloc ("playlist-dances-n", LIST_ORDERED);
  ilistSetSize (pl->pldances, ilistGetCount (tpldances));
  ilistStartIterator (tpldances, &iteridx);
  while ((tidx = ilistIterateKey (tpldances, &iteridx)) >= 0) {
    /* have to make a clone of the data */
    /* tidx is a generic key and has no relation to what was loaded */
    /* into dances.txt. Want to have pl->pldances use the danceidx as */
    /* the key. */
    didx = ilistGetNum (tpldances, tidx, PLDANCE_DANCE);
    /* skip any unknown dances */
    if (didx != LIST_VALUE_INVALID) {
      for (size_t i = 0; i < PLDANCE_KEY_MAX; ++i) {
        ilistSetNum (pl->pldances, didx, playlistdancedfkeys [i].itemkey,
            ilistGetNum (tpldances, tidx, playlistdancedfkeys [i].itemkey));
      }
    }
  }
  datafileFree (pl->pldancesdf);
  pl->pldancesdf = NULL;

  ilistDumpInfo (pl->pldances);

  type = (pltype_t) nlistGetNum (pl->plinfo, PLAYLIST_TYPE);

  if (type == PLTYPE_SONGLIST) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "songlist: load songlist %s", fname);
    pl->songlist = songlistLoad (fname);
    if (pl->songlist == NULL) {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: missing songlist %s", tfn);
      playlistFree (pl);
      return NULL;
    }

    songlistStartIterator (pl->songlist, &pl->songlistiter);
  }

  if (type == PLTYPE_SEQUENCE) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "sequence: load sequence %s", fname);
    pl->sequence = sequenceAlloc (fname);
    if (pl->sequence == NULL) {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: missing sequence %s", fname);
      playlistFree (pl);
      return NULL;
    }

    /* reset and set all of the 'selected' flags. */
    /* this just makes playlist management work better for sequences */
    playlistResetAll (pl);

    /* the sequence iterator doesn't stop, use the returned dance list */
    tlist = sequenceGetDanceList (pl->sequence);
    nlistStartIterator (tlist, &iteridx);
    while ((didx = nlistIterateKey (tlist, &iteridx)) >= 0) {
      ilistSetNum (pl->pldances, didx, PLDANCE_SELECTED, 1);
    }

    sequenceStartIterator (pl->sequence, &pl->seqiteridx);
  }

  return pl;
}

playlist_t *
playlistCreate (const char *plname, pltype_t type, musicdb_t *musicdb)
{
  playlist_t    *pl;
  char          tbuff [40];
  ilistidx_t    didx;
  dance_t       *dances;
  level_t       *levels;
  ilistidx_t    iteridx;


  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  pl = playlistAlloc (musicdb);
  pl->name = mdstrdup (plname);
  snprintf (tbuff, sizeof (tbuff), "plinfo-c-%s", plname);
  pl->plinfo = nlistAlloc (tbuff, LIST_UNORDERED, NULL);
  nlistSetSize (pl->plinfo, PLAYLIST_KEY_MAX);
  nlistSetStr (pl->plinfo, PLAYLIST_ALLOWED_KEYWORDS, NULL);
  nlistSetNum (pl->plinfo, PLAYLIST_ANNOUNCE, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_GAP, -100);
  nlistSetNum (pl->plinfo, PLAYLIST_LEVEL_HIGH, levelGetMax (levels));
  nlistSetNum (pl->plinfo, PLAYLIST_LEVEL_LOW, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_MAX_PLAY_TIME, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_RATING, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_TYPE, type);
  nlistSetNum (pl->plinfo, PLAYLIST_STOP_AFTER, 0);
  nlistSetNum (pl->plinfo, PLAYLIST_STOP_TIME, LIST_VALUE_INVALID);
  nlistSort (pl->plinfo);

  if (type == PLTYPE_SONGLIST) {
    pl->songlist = songlistLoad (plname);
  }
  if (type == PLTYPE_SEQUENCE) {
    pl->sequence = sequenceAlloc (plname);
  }

  snprintf (tbuff, sizeof (tbuff), "pldance-c-%s", plname);
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

  return pl;
}

void
playlistResetAll (playlist_t *pl)
{
  dance_t       *dances;
  ilistidx_t    iteridx;
  ilistidx_t    didx;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceStartIterator (dances, &iteridx);
  while ((didx = danceIterate (dances, &iteridx)) >= 0) {
    ilistSetNum (pl->pldances, didx, PLDANCE_SELECTED, 0);
    ilistSetNum (pl->pldances, didx, PLDANCE_COUNT, 0);
  }
}

const char *
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

  conv.str = mdstrdup (value);
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
playlistGetNextSong (playlist_t *pl,
    ssize_t priorCount, danceselQueueLookup_t queueLookupProc, void *userdata)
{
  pltype_t    type;
  song_t      *song = NULL;
  int         count;
  char        *sfname;
  int         stopAfter;


  if (pl == NULL) {
    return NULL;
  }

  logProcBegin (LOG_PROC, "playlistGetNextSong");
  type = (pltype_t) nlistGetNum (pl->plinfo, PLAYLIST_TYPE);
  stopAfter = nlistGetNum (pl->plinfo, PLAYLIST_STOP_AFTER);
  if (pl->editmode == EDIT_FALSE && stopAfter > 0 && pl->count >= stopAfter) {
    logMsg (LOG_DBG, LOG_BASIC, "pl %s stop after %d", pl->name, stopAfter);
    return NULL;
  }

  if (type == PLTYPE_AUTO || type == PLTYPE_SEQUENCE) {
    ilistidx_t     danceIdx = LIST_VALUE_INVALID;

    if (type == PLTYPE_AUTO) {
      if (pl->countList == NULL) {
        playlistCountList (pl);
      }
      if (pl->dancesel == NULL) {
        pl->dancesel = danceselAlloc (pl->countList, queueLookupProc, userdata);
      }
      danceIdx = danceselSelect (pl->dancesel, priorCount);
      logMsg (LOG_DBG, LOG_BASIC, "automatic: dance: %d/%s", danceIdx,
          danceGetStr (pl->dances, danceIdx, DANCE_DANCE));
      if (pl->songsel == NULL) {
        playlistSetSongFilter (pl);
        pl->songsel = songselAlloc (pl->musicdb,
            pl->countList, NULL, pl->songfilter);
      }
    }
    if (type == PLTYPE_SEQUENCE) {
      if (pl->songsel == NULL) {
        playlistSetSongFilter (pl);
        pl->songsel = songselAlloc (pl->musicdb,
            sequenceGetDanceList (pl->sequence), NULL, pl->songfilter);
      }
      danceIdx = sequenceIterate (pl->sequence, &pl->seqiteridx);
      logMsg (LOG_DBG, LOG_BASIC, "sequence: dance: %d/%s", danceIdx,
          danceGetStr (pl->dances, danceIdx, DANCE_DANCE));
    }

    song = songselSelect (pl->songsel, danceIdx);
    count = 0;
    while (song != NULL && count < PL_VALID_SONG_ATTEMPTS) {
      if (songAudioFileExists (song)) {
        break;
      }
      song = songselSelect (pl->songsel, danceIdx);
      ++count;
    }

    if (count >= PL_VALID_SONG_ATTEMPTS) {
      song = NULL;
    } else {
      songselSelectFinalize (pl->songsel, danceIdx);
      sfname = songGetStr (song, TAG_FILE);
      ++pl->count;
      logMsg (LOG_DBG, LOG_BASIC, "select: %d/%s %s", danceIdx,
          danceGetStr (pl->dances, danceIdx, DANCE_DANCE), sfname);
    }
  }

  if (type == PLTYPE_SONGLIST) {
    ilistidx_t    slkey;

    slkey = songlistIterate (pl->songlist, &pl->songlistiter);
    sfname = songlistGetStr (pl->songlist, slkey, SONGLIST_FILE);
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
            tstr = songlistGetStr (pl->songlist, slkey, SONGLIST_DANCESTR);
            if (tstr != NULL) {
              songSetStr (song, TAG_MQDISPLAY, tstr);
            }
          }
        }
        break;
      }
      song = NULL;
      logMsg (LOG_DBG, LOG_IMPORTANT, "WARN: songlist: missing: %s", sfname);
      slkey = songlistIterate (pl->songlist, &pl->songlistiter);
      sfname = songlistGetStr (pl->songlist, slkey, SONGLIST_FILE);
    }
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
  char        tbuff [MAXPATHLEN];
  slist_t     *filelist;
  slist_t     *pnlist;
  pathinfo_t  *pi;
  slistidx_t  iteridx;
  char        *ext = NULL;


  pnlist = slistAlloc ("playlistlist", LIST_ORDERED, NULL);

  pathbldMakePath (tfn, sizeof (tfn), "", "", PATHBLD_MP_DREL_DATA);
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
  filelist = dirlistBasicDirList (tfn, ext);

  slistStartIterator (filelist, &iteridx);
  while ((tplfnm = slistIterateKey (filelist, &iteridx)) != NULL) {
    pi = pathInfo (tplfnm);
    strlcpy (tfn, pi->basename, pi->blen + 1);
    tfn [pi->blen] = '\0';

    pathInfoFree (pi);
    if ((flag == PL_LIST_NORMAL || flag == PL_LIST_AUTO_SEQ) &&
        /* CONTEXT: playlist: the name for the special playlist used for the 'queue dance' button */
        strcmp (tfn, _("QueueDance")) == 0) {
      continue;
    }
    if (flag == PL_LIST_AUTO_SEQ) {
      pathbldMakePath (tbuff, sizeof (tbuff), tfn, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
      if (fileopFileExists (tbuff)) {
        continue;
      }
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
    dataFree (pl->name);
    pl->name = mdstrdup (name);
  }

  pathbldMakePath (tfn, sizeof (tfn), pl->name,
      BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
  datafileSaveKeyVal ("playlist", tfn, playlistdfkeys,
      PLAYLIST_KEY_MAX, pl->plinfo, 0);

  pathbldMakePath (tfn, sizeof (tfn), pl->name,
      BDJ4_PL_DANCE_EXT, PATHBLD_MP_DREL_DATA);
  datafileSaveIndirect ("playlist-dances", tfn, playlistdancedfkeys,
      PLDANCE_KEY_MAX, pl->pldances);
}

void
playlistSetEditMode (playlist_t *pl, int editmode)
{
  if (pl == NULL) {
    return;
  }

  pl->editmode = editmode;
}

int
playlistGetEditMode (playlist_t *pl)
{
  if (pl == NULL) {
    return EDIT_FALSE;
  }

  return pl->editmode;
}

bool
playlistExists (const char *name)
{
  char  tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff),
      name, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
  return fileopFileExists (tbuff);
}

void
playlistRename (const char *oldname, const char *newname)
{
  char  onm [MAXPATHLEN];
  char  nnm [MAXPATHLEN];

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DREL_DATA);
  filemanipRenameAll (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
  filemanipRenameAll (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
  filemanipRenameAll (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DREL_DATA);
  filemanipRenameAll (onm, nnm);
}

void
playlistCheckAndCreate (const char *name, pltype_t pltype)
{
  char  onm [MAXPATHLEN];

  pathbldMakePath (onm, sizeof (onm),
      name, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (onm)) {
    playlist_t    *pl;

    pl = playlistCreate (name, pltype, NULL);
    if (pl != NULL) {
      playlistSave (pl, NULL);
      playlistFree (pl);
    }
  }
}

void
playlistDelete (const char *name)
{
  char  tnm [MAXPATHLEN];

  pathbldMakePath (tnm, sizeof (tnm),
      name, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
  filemanipDeleteAll (tnm);
  pathbldMakePath (tnm, sizeof (tnm),
      name, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DREL_DATA);
  filemanipDeleteAll (tnm);
  pathbldMakePath (tnm, sizeof (tnm),
      name, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
  filemanipDeleteAll (tnm);
  pathbldMakePath (tnm, sizeof (tnm),
      name, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DREL_DATA);
  filemanipDeleteAll (tnm);
}

void
playlistCopy (const char *oldname, const char *newname)
{
  char  onm [MAXPATHLEN];
  char  nnm [MAXPATHLEN];

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
  filemanipCopy (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DREL_DATA);
  filemanipCopy (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_SEQUENCE_EXT, PATHBLD_MP_DREL_DATA);
  filemanipCopy (onm, nnm);

  pathbldMakePath (onm, sizeof (onm),
      oldname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
  pathbldMakePath (nnm, sizeof (nnm),
      newname, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
  filemanipCopy (onm, nnm);
}

/* internal routines */

static playlist_t *
playlistAlloc (musicdb_t *musicdb)
{
  playlist_t    *pl = NULL;

  pl = mdmalloc (sizeof (playlist_t));
  pl->name = NULL;
  pl->songlistiter = 0;
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
  pl->editmode = EDIT_FALSE;
  pl->musicdb = musicdb;
  pl->dances = bdjvarsdfGet (BDJVDF_DANCES);

  return pl;
}

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
  /* the allowed keywords list needs to be sorted for the song filter */
  slistSort (kwList);
  /* the keyword list is always set as in-use, so that songs with */
  /* keywords will be rejected.  It's ok to send a null kwList. */
  songfilterSetData (pl->songfilter, SONG_FILTER_KEYWORD, kwList);

  danceList = ilistAlloc ("pl-dance-filter", LIST_ORDERED);
  /* the songfilter must have the dance-list set before the bpm settings */
  /* are applied */
  songfilterSetData (pl->songfilter, SONG_FILTER_DANCE_LIST, danceList);

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
}

static void
playlistCountList (playlist_t *pl)
{
  int         sel;
  dbidx_t     count;
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
      nlistSetNum (pl->countList, didx, count);
    }
  }
  logProcEnd (LOG_PROC, "playlistCountList", "");
}
