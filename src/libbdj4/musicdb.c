/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "filemanip.h"
#include "fileop.h"
#include "lock.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathbld.h"
#include "rafile.h"
#include "slist.h"
#include "song.h"
#include "songutil.h"
#include "tagdef.h"
#include "tmutil.h"

typedef struct musicdb {
  dbidx_t       count;
  nlist_t       *songs;
  nlist_t       *danceCounts;  // used by main for automatic playlists
  dbidx_t       danceCount;
  rafile_t      *radb;
  char          *fn;
  nlist_t       *tempSongs;
  bool          inbatch;
  bool          updatelast;
} musicdb_t;

static size_t dbWriteInternal (musicdb_t *musicdb, const char *fn, slist_t *tagList, dbidx_t rrn);
static song_t *dbReadEntry (musicdb_t *musicdb, rafileidx_t rrn);

musicdb_t *
dbOpen (const char *fn)
{
  dance_t       *dances;
  dbidx_t       dcount;
  musicdb_t     *musicdb;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dcount = danceGetCount (dances);

  musicdb = mdmalloc (sizeof (musicdb_t));

  musicdb->songs = slistAlloc ("db-songs", LIST_UNORDERED, songFree);
  musicdb->danceCounts = nlistAlloc ("db-dance-counts", LIST_ORDERED, NULL);
  nlistSetSize (musicdb->danceCounts, dcount);
  musicdb->danceCount = dcount;
  musicdb->count = 0L;
  musicdb->radb = NULL;
  musicdb->inbatch = false;
  musicdb->updatelast = true;
  musicdb->fn = mdstrdup (fn);
  /* tempsongs is ordered by dbidx */
  musicdb->tempSongs = nlistAlloc ("db-temp-songs", LIST_ORDERED, songFree);
  dbLoad (musicdb);

  return musicdb;
}

void
dbClose (musicdb_t *musicdb)
{
  /* for each song in db, free the song */
  if (musicdb != NULL) {
    slistFree (musicdb->songs);
    nlistFree (musicdb->danceCounts);
    dataFree (musicdb->fn);
    raClose (musicdb->radb);
    musicdb->radb = NULL;
    nlistFree (musicdb->tempSongs);
    mdfree (musicdb);
  }
  musicdb = NULL;
}

dbidx_t
dbCount (musicdb_t *musicdb)
{
  dbidx_t tcount = 0L;
  if (musicdb != NULL) {
    tcount = musicdb->count;
  }
  return tcount;
}

int
dbLoad (musicdb_t *musicdb)
{
  char        data [RAFILE_REC_SIZE];
  char        *fstr;
  char        *ffn;
  song_t      *song;
  rafileidx_t rc;
  nlistidx_t  dkey;
  nlistidx_t  iteridx;
  slistidx_t  dbidx;
  slistidx_t  siteridx;
  bool        ok;


  fstr = "";
  musicdb->radb = raOpen (musicdb->fn, MUSICDB_VERSION);
  slistSetSize (musicdb->songs, raGetCount (musicdb->radb));

  raStartBatch (musicdb->radb);

  for (rafileidx_t i = 1L; i <= raGetCount (musicdb->radb); ++i) {
    /* should be able to replace the below code with dbReadEntry() */
    /* but bdj4main crashes, and I don't know why */
    rc = raRead (musicdb->radb, i, data);
    if (rc != 1) {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: Unable to access rrn %d", i);
    }
    if (rc == 0 || ! *data) {
      continue;
    }

    song = songAlloc ();
    songParse (song, data, i);
    fstr = songGetStr (song, TAG_FILE);
    ffn = songutilFullFileName (fstr);
    ok = false;
    if (fileopFileExists (ffn)) {
      ok = true;
    } else {
      logMsg (LOG_DBG, LOG_IMPORTANT, "song %s not found", fstr);
      songFree (song);
      song = NULL;
    }
    mdfree (ffn);

    if (ok) {
      dkey = songGetNum (song, TAG_DANCE);
      if (dkey >= 0) {
        nlistIncrement (musicdb->danceCounts, dkey);
      }
      songSetNum (song, TAG_RRN, i);
      slistSetData (musicdb->songs, fstr, song);
      ++musicdb->count;
    }
  }

  /* sort so that lookups can be done by file name */
  slistSort (musicdb->songs);

  /* set the database index according to the sorted values */
  dbidx = 0;
  slistStartIterator (musicdb->songs, &siteridx);
  while ((song = slistIterateValueData (musicdb->songs, &siteridx)) != NULL) {
    songSetNum (song, TAG_DBIDX, dbidx);
    ++dbidx;
  }

  /* debug information */
  nlistStartIterator (musicdb->danceCounts, &iteridx);
  while ((dkey = nlistIterateKey (musicdb->danceCounts, &iteridx)) >= 0) {
    dbidx_t count = nlistGetNum (musicdb->danceCounts, dkey);
    if (count > 0) {
      logMsg (LOG_DBG, LOG_DB, "db-load: dance: %d count: %d", dkey, count);
    }
  }

  raEndBatch (musicdb->radb);
  raClose (musicdb->radb);
  musicdb->radb = NULL;
  return 0;
}

void
dbLoadEntry (musicdb_t *musicdb, dbidx_t dbidx)
{
  song_t      *song;
  rafileidx_t rrn;
  char        *fstr;

  if (musicdb->radb == NULL) {
    musicdb->radb = raOpen (musicdb->fn, MUSICDB_VERSION);
  }
  song = slistGetDataByIdx (musicdb->songs, dbidx);
  rrn = songGetNum (song, TAG_RRN);
  song = dbReadEntry (musicdb, rrn);
  fstr = songGetStr (song, TAG_FILE);
  songSetNum (song, TAG_RRN, rrn);
  songSetNum (song, TAG_DBIDX, dbidx);
  if (song != NULL) {
    slistSetData (musicdb->songs, fstr, song);
  }
}

void
dbStartBatch (musicdb_t *musicdb)
{
  if (musicdb == NULL) {
    return;
  }

  if (musicdb->radb == NULL) {
    musicdb->radb = raOpen (musicdb->fn, MUSICDB_VERSION);
  }
  raStartBatch (musicdb->radb);
  musicdb->inbatch = true;
}

void
dbEndBatch (musicdb_t *musicdb)
{
  if (musicdb == NULL) {
    return;
  }

  raEndBatch (musicdb->radb);
  raClose (musicdb->radb);
  musicdb->radb = NULL;
  musicdb->inbatch = false;
}

void
dbDisableLastUpdateTime (musicdb_t *musicdb)
{
  if (musicdb == NULL) {
    return;
  }

  musicdb->updatelast = false;
}

void
dbEnableLastUpdateTime (musicdb_t *musicdb)
{
  if (musicdb == NULL) {
    return;
  }

  musicdb->updatelast = true;
}

song_t *
dbGetByName (musicdb_t *musicdb, const char *songname)
{
  song_t *song = slistGetData (musicdb->songs, songname);
  return song;
}

song_t *
dbGetByIdx (musicdb_t *musicdb, dbidx_t idx)
{
  song_t  *song;

  if (idx < 0) {
    return NULL;
  }

  if (idx < musicdb->count) {
    song = slistGetDataByIdx (musicdb->songs, idx);
  } else {
    /* check the temporary list if the dbidx is greater than the count */
    song = nlistGetData (musicdb->tempSongs, idx);
  }
  return song;
}

size_t
dbWriteSong (musicdb_t *musicdb, song_t *song)
{
  slist_t   *taglist;
  bool      rc;
  time_t    currtime;

  if (song == NULL) {
    return 0;
  }

  if (songGetNum (song, TAG_TEMPORARY) == true) {
    return 0;
  }

  if (musicdb->updatelast) {
    currtime = time (NULL);
    songSetNum (song, TAG_LAST_UPDATED, currtime);
  }
  taglist = songTagList (song);
  rc = dbWriteInternal (musicdb, songGetStr (song, TAG_FILE),
      taglist, songGetNum (song, TAG_RRN));
  slistFree (taglist);
  return rc;
}

size_t
dbWrite (musicdb_t *musicdb, const char *fn, slist_t *tagList, dbidx_t rrn)
{
  size_t    tblen;
  time_t    currtime;

  if (musicdb->updatelast) {
    char tmp [40];

    currtime = time (NULL);
    snprintf (tmp, sizeof (tmp), "%" PRId64, (int64_t) currtime);
    slistSetStr (tagList, tagdefs [TAG_LAST_UPDATED].tag, tmp);
  }
  tblen = dbWriteInternal (musicdb, fn, tagList, rrn);
  return tblen;
}

size_t
dbCreateSongEntryFromTags (char *tbuff, size_t sz, slist_t *tagList,
    const char *fn, dbidx_t rrn)
{
  size_t        tblen = 0;
  slistidx_t    iteridx;
  char          *tag;
  char          *data;
  bool          havestatus = false;
  char          tmp [100];


  tbuff [0] = '\0';

  tblen = stringAppend (tbuff, sz, tblen, tagdefs [TAG_FILE].tag);
  tblen = stringAppend (tbuff, sz, tblen, "\n");
  tblen = stringAppend (tbuff, sz, tblen, "..");
  tblen = stringAppend (tbuff, sz, tblen, fn);
  tblen = stringAppend (tbuff, sz, tblen, "\n");

  slistStartIterator (tagList, &iteridx);
  while ((tag = slistIterateKey (tagList, &iteridx)) != NULL) {
    if (strcmp (tag, tagdefs [TAG_FILE].tag) == 0) {
      /* already handled, must be first */
      continue;
    }
    if (strcmp (tag, tagdefs [TAG_STATUS].tag) == 0) {
      havestatus = true;
    }

    data = slistGetStr (tagList, tag);

    if (data == NULL || *data == '\0') {
      /* do not write null data */
      continue;
    }

    tblen = stringAppend (tbuff, sz, tblen, tag);
    tblen = stringAppend (tbuff, sz, tblen, "\n");
    tblen = stringAppend (tbuff, sz, tblen, "..");
    tblen = stringAppend (tbuff, sz, tblen, data);
    tblen = stringAppend (tbuff, sz, tblen, "\n");
  }

  if (rrn == MUSICDB_ENTRY_NEW) {
    tblen = stringAppend (tbuff, sz, tblen, tagdefs [TAG_DBADDDATE].tag);
    tblen = stringAppend (tbuff, sz, tblen, "\n");
    tblen = stringAppend (tbuff, sz, tblen, "..");
    tmutilDstamp (tmp, sizeof (tmp));
    tblen = stringAppend (tbuff, sz, tblen, tmp);
    tblen = stringAppend (tbuff, sz, tblen, "\n");
  }

  if (! havestatus) {
    tblen = stringAppend (tbuff, sz, tblen, tagdefs [TAG_STATUS].tag);
    tblen = stringAppend (tbuff, sz, tblen, "\n");
    tblen = stringAppend (tbuff, sz, tblen, "..");
    /* CONTEXT: music database: default status */
    tblen = stringAppend (tbuff, sz, tblen, _("New"));
    tblen = stringAppend (tbuff, sz, tblen, "\n");
  }

  return tblen;
}

void
dbStartIterator (musicdb_t *musicdb, slistidx_t *iteridx)
{
  if (musicdb == NULL) {
    return;
  }
  if (musicdb->songs == NULL) {
    return;
  }

  slistStartIterator (musicdb->songs, iteridx);
}

song_t *
dbIterate (musicdb_t *musicdb, dbidx_t *idx, slistidx_t *iteridx)
{
  song_t    *song;

  if (musicdb == NULL) {
    return NULL;
  }

  song = slistIterateValueData (musicdb->songs, iteridx);
  *idx = slistIterateGetIdx (musicdb->songs, iteridx);
  return song;
}

nlist_t *
dbGetDanceCounts (musicdb_t *musicdb)
{
  return musicdb->danceCounts;
}

void
dbBackup (void)
{
  char  dbfname [MAXPATHLEN];

  pathbldMakePath (dbfname, sizeof (dbfname),
      MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_DREL_DATA);
  filemanipBackup (dbfname, 4);
}

dbidx_t
dbAddTemporarySong (musicdb_t *musicdb, song_t *song)
{
  dbidx_t     dbidx;

  if (song == NULL) {
    return LIST_VALUE_INVALID;
  }

  songSetNum (song, TAG_TEMPORARY, true);
  songGetStr (song, TAG_FILE);
  dbidx = musicdb->count;
  dbidx += nlistGetCount (musicdb->tempSongs);
  songSetNum (song, TAG_DBIDX, dbidx);
  nlistSetData (musicdb->tempSongs, dbidx, song);
  return dbidx;
}

/* internal routines */

static size_t
dbWriteInternal (musicdb_t *musicdb, const char *fn,
    slist_t *tagList, dbidx_t rrn)
{
  char          tbuff [RAFILE_REC_SIZE];
  size_t        tblen;

  if (musicdb == NULL) {
    return false;
  }

  if (musicdb->radb == NULL) {
    musicdb->radb = raOpen (musicdb->fn, MUSICDB_VERSION);
  }

  tblen = dbCreateSongEntryFromTags (tbuff, sizeof (tbuff), tagList, fn, rrn);
  raWrite (musicdb->radb, rrn, tbuff);

  if (! musicdb->inbatch) {
// ### FIX
// figure out what's going on here.
// this should not be necessary.
    /* this is inefficient, but otherwise the disk buffering / file handling */
    /* causes issues w/reading an updated entry */
    raClose (musicdb->radb);
    musicdb->radb = NULL;
  }
  return tblen;
}

static song_t *
dbReadEntry (musicdb_t *musicdb, rafileidx_t rrn)
{
  int     rc;
  song_t  *song;
  char    data [RAFILE_REC_SIZE];

  *data = '\0';
  rc = raRead (musicdb->radb, rrn, data);
  if (rc != 1) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: Unable to access rrn %d", rrn);
  }
  if (rc == 0 || ! *data) {
    return NULL;
  }

  song = songAlloc ();
  songParse (song, data, rrn);
  if (! songAudioFileExists (song)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "song %s not found",
        songGetStr (song, TAG_FILE));
    songFree (song);
    song = NULL;
  }

  return song;
}
