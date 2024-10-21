/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "filemanip.h"
#include "fileop.h"
#include "ilist.h"
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

enum {
  MUSICDB_IDENT = 0xcc0062646973756d,
  MUSICDB_TEMP_OFFSET = 10000,
};

typedef struct musicdb {
  uint64_t      ident;
  dbidx_t       count;
  slist_t       *songbyname;
  nlist_t       *songbyidx;
  nlist_t       *danceCounts;  // used by main for automatic playlists
  dbidx_t       danceCount;
  rafile_t      *radb;
  char          *fn;
  nlist_t       *tempSongs;
  bool          inbatch;
  bool          updatelast;
} musicdb_t;

static size_t dbWriteInternalSong (musicdb_t *musicdb, const char *fn, song_t *song, dbidx_t rrn);
static song_t *dbReadEntry (musicdb_t *musicdb, rafileidx_t rrn);
static void   dbRebuildDanceCounts (musicdb_t *musicdb);

musicdb_t *
dbOpen (const char *fn)
{
  dance_t       *dances;
  dbidx_t       dcount;
  musicdb_t     *musicdb;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  dcount = danceGetCount (dances);

  musicdb = mdmalloc (sizeof (musicdb_t));

  musicdb->ident = MUSICDB_IDENT;
  musicdb->songbyname = slistAlloc ("db-song-name", LIST_UNORDERED, NULL);
  musicdb->songbyidx = nlistAlloc ("db-song-idx", LIST_UNORDERED, songFree);
  musicdb->danceCounts = nlistAlloc ("db-dance-counts", LIST_ORDERED, NULL);
  nlistSetSize (musicdb->danceCounts, dcount);
  musicdb->danceCount = dcount;
  musicdb->count = 0;
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
  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return;
  }

  if (musicdb->inbatch) {
    dbEndBatch (musicdb);
  }
  raClose (musicdb->radb);
  musicdb->radb = NULL;

  slistFree (musicdb->songbyname);
  nlistFree (musicdb->songbyidx);
  nlistFree (musicdb->danceCounts);
  dataFree (musicdb->fn);
  nlistFree (musicdb->tempSongs);
  musicdb->ident = BDJ4_IDENT_FREE;
  mdfree (musicdb);
}

dbidx_t
dbCount (musicdb_t *musicdb)
{
  dbidx_t tcount = 0;

  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return 0;
  }

  tcount = musicdb->count;
  return tcount;
}

int
dbLoad (musicdb_t *musicdb)
{
  song_t      *song;
  slist_t     *templist;
  nlistidx_t  iteridx;
  slistidx_t  dbidx;
  slistidx_t  siteridx;
  rafileidx_t racount;

  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return -1;
  }

  musicdb->radb = raOpen (musicdb->fn, MUSICDB_VERSION);
  racount = raGetCount (musicdb->radb);
  /* the songs loaded into the templist will be re-assigned to */
  /* the songbyidx list, and should not be freed */
  templist = slistAlloc ("db-temp", LIST_UNORDERED, NULL);
  slistSetSize (templist, racount);
  slistSetSize (musicdb->songbyname, racount);
  nlistSetSize (musicdb->songbyidx, racount);
  logMsg (LOG_DBG, LOG_DB, "db-load: %s %" PRId32 "\n", musicdb->fn, racount);

  raStartBatch (musicdb->radb);

  /* the random access file is indexed starting at 1 */
  for (rafileidx_t i = 1; i <= racount; ++i) {
    song = dbReadEntry (musicdb, i);

    if (song != NULL) {
      const char  *uri = NULL;
      nlistidx_t  dkey;

      dkey = songGetNum (song, TAG_DANCE);
      if (dkey >= 0) {
        nlistIncrement (musicdb->danceCounts, dkey);
      }
      songSetNum (song, TAG_RRN, i);
      uri = songGetStr (song, TAG_URI);
      slistSetData (templist, uri, song);
      ++musicdb->count;
    }
  }

  /* sort so that lookups can be done by uri */
  slistSort (templist);

  /* set the database index according to the sorted values */
  /* a dual list setup is used so that the song data is easily updated */
  /* on a rename */

  dbidx = 0;
  slistStartIterator (templist, &siteridx);
  while ((song = slistIterateValueData (templist, &siteridx)) != NULL) {
    slistSetNum (musicdb->songbyname, songGetStr (song, TAG_URI), dbidx);
    nlistSetData (musicdb->songbyidx, dbidx, song);
    songSetNum (song, TAG_DBIDX, dbidx);
    songSetNum (song, TAG_DB_FLAGS, MUSICDB_STD);
    ++dbidx;
  }

  slistSort (musicdb->songbyname);
  nlistSort (musicdb->songbyidx);

  if (logCheck (LOG_DBG, LOG_DB)) {
    nlistidx_t  dkey;

    /* debug information */
    nlistStartIterator (musicdb->danceCounts, &iteridx);
    while ((dkey = nlistIterateKey (musicdb->danceCounts, &iteridx)) >= 0) {
      dbidx_t count = nlistGetNum (musicdb->danceCounts, dkey);
      if (count > 0) {
        logMsg (LOG_DBG, LOG_DB, "db-load: dance: %d count: %" PRId32, dkey, count);
      }
    }
  }

  slistFree (templist);

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

  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return;
  }

  if (musicdb->radb == NULL) {
    musicdb->radb = raOpen (musicdb->fn, MUSICDB_VERSION);
  }

  /* old entry */
  song = nlistGetData (musicdb->songbyidx, dbidx);
  rrn = songGetNum (song, TAG_RRN);

  song = dbReadEntry (musicdb, rrn);

  if (song != NULL) {
    songSetNum (song, TAG_RRN, rrn);
    songSetNum (song, TAG_DBIDX, dbidx);
    nlistSetData (musicdb->songbyidx, dbidx, song);
  }
}

void
dbMarkEntryRenamed (musicdb_t *musicdb, const char *olduri,
    const char *newuri, dbidx_t dbidx)
{
  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return;
  }

  slistSetNum (musicdb->songbyname, olduri, LIST_VALUE_INVALID);
  slistSetNum (musicdb->songbyname, newuri, dbidx);
}

/* marks the entry as removed, but does not actually remove it */
void
dbMarkEntryRemoved (musicdb_t *musicdb, dbidx_t dbidx)
{
  song_t      *song;

  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return;
  }

  if (musicdb->radb == NULL) {
    musicdb->radb = raOpen (musicdb->fn, MUSICDB_VERSION);
  }
  song = nlistGetData (musicdb->songbyidx, dbidx);
  songSetNum (song, TAG_DB_FLAGS, MUSICDB_REMOVED);
  dbRebuildDanceCounts (musicdb);
}

/* clears the removed mark */
void
dbClearEntryRemoved (musicdb_t *musicdb, dbidx_t dbidx)
{
  song_t      *song;

  if (musicdb == NULL) {
    return;
  }

  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    musicdb->radb = raOpen (musicdb->fn, MUSICDB_VERSION);
  }
  song = nlistGetData (musicdb->songbyidx, dbidx);
  songSetNum (song, TAG_DB_FLAGS, MUSICDB_STD);
  dbRebuildDanceCounts (musicdb);
}

void
dbStartBatch (musicdb_t *musicdb)
{
  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
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
  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return;
  }

  raEndBatch (musicdb->radb);
  musicdb->inbatch = false;
}

void
dbDisableLastUpdateTime (musicdb_t *musicdb)
{
  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return;
  }

  logMsg (LOG_DBG, LOG_IMPORTANT, "INFO: database disabled last update time");
  musicdb->updatelast = false;
}

void
dbEnableLastUpdateTime (musicdb_t *musicdb)
{
  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return;
  }

  logMsg (LOG_DBG, LOG_IMPORTANT, "INFO: database enabled last update time");
  musicdb->updatelast = true;
}

song_t *
dbGetByName (musicdb_t *musicdb, const char *songname)
{
  song_t  *song = NULL;
  dbidx_t dbidx;

  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return NULL;
  }

  dbidx = slistGetNum (musicdb->songbyname, songname);
  if (dbidx >= 0) {
    song = nlistGetData (musicdb->songbyidx, dbidx);
    if (songGetNum (song, TAG_DB_FLAGS) == MUSICDB_REMOVED) {
      song = NULL;
    }
  }

  return song;
}

song_t *
dbGetByIdx (musicdb_t *musicdb, dbidx_t idx)
{
  song_t  *song;

  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return NULL;
  }

  if (idx < 0) {
    return NULL;
  }

  if (idx < musicdb->count) {
    song = nlistGetData (musicdb->songbyidx, idx);
    if (songGetNum (song, TAG_DB_FLAGS) == MUSICDB_REMOVED) {
      song = NULL;
    }
  } else {
    /* check the temporary list if the dbidx is greater than the count */
    song = nlistGetData (musicdb->tempSongs, idx);
  }
  return song;
}

size_t
dbWriteSong (musicdb_t *musicdb, song_t *song)
{
  time_t      currtime;
  size_t      len;
  const char  *uri;

  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return 0;
  }

  if (song == NULL) {
    return 0;
  }

  /* do not write temporary or removed songs */
  if (songGetNum (song, TAG_DB_FLAGS) != MUSICDB_STD) {
    return 0;
  }

  uri = songGetStr (song, TAG_URI);

  if (musicdb->updatelast) {
    currtime = time (NULL);
    songSetNum (song, TAG_LAST_UPDATED, currtime);
  }
  len = dbWriteInternalSong (musicdb, uri,
      song, songGetNum (song, TAG_RRN));
  return len;
}

size_t
dbCreateSongEntryFromSong (char *tbuff, size_t sz, song_t *song,
    const char *fn)
{
  char        *sbuff;
  int         tstatus;
  ssize_t     tval;

  tbuff [0] = '\0';

  if (song == NULL) {
    return 0;
  }

  /* make sure the db-add-date is set */
  tval = songGetNum (song, TAG_DBADDDATE);
  if (tval <= 0) {
    songSetNum (song, TAG_DBADDDATE, time (NULL));
  }

  /* make sure a status is set */
  tstatus = songGetNum (song, TAG_STATUS);
  if (tstatus < 0) {
    songSetNum (song, TAG_STATUS, 0);
  }

  sbuff = songCreateSaveData (song);
  stpecpy (tbuff, tbuff + sz, sbuff);
  mdfree (sbuff);

  return -1;
}

void
dbStartIterator (musicdb_t *musicdb, slistidx_t *iteridx)
{
  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return;
  }
  if (musicdb->songbyname == NULL) {
    return;
  }

  slistStartIterator (musicdb->songbyname, iteridx);
}

/* iterates by dbidx */
/* iterating by name isn't necessary, as all displays use */
/* the song filter process */
song_t *
dbIterate (musicdb_t *musicdb, dbidx_t *dbidx, slistidx_t *iteridx)
{
  song_t    *song;

  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return NULL;
  }

  *dbidx = nlistIterateKey (musicdb->songbyidx, iteridx);
  song = nlistGetData (musicdb->songbyidx, *dbidx);
  while (songGetNum (song, TAG_DB_FLAGS) == MUSICDB_REMOVED) {
    *dbidx = nlistIterateKey (musicdb->songbyidx, iteridx);
    song = nlistGetData (musicdb->songbyidx, *dbidx);
  }
  return song;
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

  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return LIST_VALUE_INVALID;
  }
  if (song == NULL) {
    return LIST_VALUE_INVALID;
  }

  songSetNum (song, TAG_DB_FLAGS, MUSICDB_TEMP);
  /* offset the temporary song dbidx so that new songs can be added */
  /* to the database without duplicating the temporary song dbidx */
  dbidx = musicdb->count + MUSICDB_TEMP_OFFSET;
  dbidx += nlistGetCount (musicdb->tempSongs);
  songSetNum (song, TAG_DBIDX, dbidx);
  nlistSetData (musicdb->tempSongs, dbidx, song);
  return dbidx;
}

dbidx_t
dbAddTemporarySongToDatabase (musicdb_t *musicdb, dbidx_t tempdbidx)
{
  dbidx_t     ndbidx;
  song_t      *song;

  if (musicdb == NULL || musicdb->ident != MUSICDB_IDENT) {
    return LIST_VALUE_INVALID;
  }

  song = nlistGetData (musicdb->tempSongs, tempdbidx);
  if (song == NULL) {
    return LIST_VALUE_INVALID;
  }
  if (songGetNum (song, TAG_DB_FLAGS) != MUSICDB_TEMP) {
    return LIST_VALUE_INVALID;
  }

  songSetNum (song, TAG_DB_FLAGS, MUSICDB_STD);
  ndbidx = musicdb->count;
  songSetNum (song, TAG_DBIDX, ndbidx);
  songSetNum (song, TAG_RRN, MUSICDB_ENTRY_NEW);
  dbWriteSong (musicdb, song);
  nlistSetData (musicdb->tempSongs, tempdbidx, NULL);
  return ndbidx;
}

#if 0 /* for debugging */
void
dbDumpSongList (musicdb_t *musicdb)   /* KEEP */
{
  slistidx_t    siteridx;
  const char    *key;
  song_t        *song;

  slistStartIterator (musicdb->songbyname, &siteridx);
  while ((key = slistIterateKey (musicdb->songbyname, &siteridx)) != NULL) {
    dbidx_t   dbidx;

    dbidx = slistGetNum (musicdb->songbyname, key);
    fprintf (stderr, "key: %s % " PRId32 "\n", key, dbidx);
    song = nlistGetData (musicdb->songbyidx, dbidx);
    fprintf (stderr, "  song: %s\n", songGetStr (song, TAG_URI));
  }
}
#endif

/* internal routines */

static size_t
dbWriteInternalSong (musicdb_t *musicdb, const char *fn,
    song_t *song, dbidx_t rrn)
{
  char          tbuff [RAFILE_REC_SIZE];
  size_t        len;

  if (musicdb == NULL) {
    return 0;
  }

  if (musicdb->radb == NULL) {
    musicdb->radb = raOpen (musicdb->fn, MUSICDB_VERSION);
  }

  dbCreateSongEntryFromSong (tbuff, sizeof (tbuff), song, fn);
  len = raWrite (musicdb->radb, rrn, tbuff, -1);

  return len;
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
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: Unable to access rrn %" PRId32, rrn);
  }
  if (rc == 0 || ! *data) {
    return NULL;
  }

  song = songAlloc ();
  songParse (song, data, rrn);
  if (! songAudioSourceExists (song)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "WARN: song %s not found",
        songGetStr (song, TAG_URI));
    songFree (song);
    song = NULL;
  }

  return song;
}

static void
dbRebuildDanceCounts (musicdb_t *musicdb)
{
  song_t        *song;
  nlistidx_t    iteridx;

  nlistFree (musicdb->danceCounts);
  musicdb->danceCounts = nlistAlloc ("db-dance-counts", LIST_ORDERED, NULL);
  nlistStartIterator (musicdb->songbyidx, &iteridx);
  while ((song = nlistIterateValueData (musicdb->songbyidx, &iteridx)) != NULL) {
    ilistidx_t    dkey;

    if (songGetNum (song, TAG_DB_FLAGS) != MUSICDB_STD) {
      continue;
    }

    dkey = songGetNum (song, TAG_DANCE);
    if (dkey >= 0) {
      nlistIncrement (musicdb->danceCounts, dkey);
    }
  }

  if (logCheck (LOG_DBG, LOG_DB)) {
    nlistidx_t    dkey;

    /* debug information */
    nlistStartIterator (musicdb->danceCounts, &iteridx);
    while ((dkey = nlistIterateKey (musicdb->danceCounts, &iteridx)) >= 0) {
      dbidx_t count = nlistGetNum (musicdb->danceCounts, dkey);
      if (count > 0) {
        logMsg (LOG_DBG, LOG_DB, "db-rebuild: dance: %" PRId32 " count: %" PRId32, dkey, count);
      }
    }
  }
}
