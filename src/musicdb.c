#include "config.h"


#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "fileop.h"
#include "nlist.h"
#include "slist.h"
#include "lock.h"
#include "log.h"
#include "musicdb.h"
#include "rafile.h"
#include "song.h"
#include "songutil.h"
#include "tagdef.h"

/* globals */
static int         initialized = 0;
static db_t        *musicdb = NULL;

void
dbOpen (char *fn)
{
  if (! initialized) {
    dance_t       *dances;
    ssize_t       dcount;

    dances = bdjvarsdfGet (BDJVDF_DANCES);
    dcount = danceGetCount (dances);

    musicdb = malloc (sizeof (db_t));
    assert (musicdb != NULL);
    musicdb->songs = slistAlloc ("db-songs", LIST_UNORDERED, songFree);
    musicdb->danceCounts = nlistAlloc ("db-dance-counts", LIST_ORDERED, NULL);
    nlistSetSize (musicdb->danceCounts, dcount);
    musicdb->danceCount = dcount;
    musicdb->count = 0L;
    dbLoad (musicdb, fn);
    initialized = 1;
  }
}

void
dbClose (void)
{
  /* for each song in db, free the song */
  if (musicdb != NULL) {
    if (musicdb->songs != NULL) {
      slistFree (musicdb->songs);
    }
    if (musicdb->danceCounts != NULL) {
      nlistFree (musicdb->danceCounts);
    }
    free (musicdb);
  }
  initialized = 0;
  musicdb = NULL;
}

ssize_t
dbCount (void)
{
  ssize_t tcount = 0L;
  if (musicdb != NULL) {
    tcount = musicdb->count;
  }
  return tcount;
}

int
dbLoad (db_t *db, char *fn)
{
  char        data [RAFILE_REC_SIZE];
  char        *fstr;
  char        *ffn;
  song_t      *song;
  rafile_t    *radb;
  rafileidx_t srrn;
  rafileidx_t rc;
  nlistidx_t  dkey;
  nlistidx_t  iteridx;
  slistidx_t  dbidx;
  slistidx_t  siteridx;
  bool        ok;


  fstr = "";
  radb = raOpen (fn, 10);
  slistSetSize (db->songs, raGetCount (radb));

  raStartBatch (radb);

  for (rafileidx_t i = 1L; i <= raGetCount (radb); ++i) {
    rc = raRead (radb, i, data);
    if (rc != 1) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: Unable to access rrn %zd", i);
    }
    if (rc == 0 || ! *data) {
      continue;
    }

    song = songAlloc ();
    songParse (song, data, i);
    fstr = songGetStr (song, TAG_FILE);
    ffn = songFullFileName (fstr);
    ok = false;
    if (fileopFileExists (ffn)) {
      ok = true;
    }
    free (ffn);

    if (! ok) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "song %s not found", fstr);
    }

    if (ok) {
      srrn = songGetNum (song, TAG_RRN);
      dkey = songGetNum (song, TAG_DANCE);
      if (dkey >= 0) {
        nlistIncrement (db->danceCounts, dkey);
      }
      if (i != srrn) {
        /* a double check to make sure the song has the correct rrn */
        songSetNum (song, TAG_RRN, i);
      }
      slistSetData (db->songs, fstr, song);
    }
    ++db->count;
  }
  slistSort (db->songs);

  /* set the database index according to the sorted values */
  dbidx = 0;
  slistStartIterator (db->songs, &siteridx);
  while ((song = slistIterateValueData (db->songs, &siteridx)) != NULL) {
    songSetNum (song, TAG_DBIDX, dbidx);
    ++dbidx;
  }

  nlistStartIterator (db->danceCounts, &iteridx);
  while ((dkey = nlistIterateKey (db->danceCounts, &iteridx)) >= 0) {
    ssize_t count = nlistGetNum (db->danceCounts, dkey);
    if (count > 0) {
      logMsg (LOG_DBG, LOG_BASIC, "db-load: dance: %zd count: %zd", dkey, count);
    }
  }

  raEndBatch (radb);
  raClose (radb);
  return 0;
}

song_t *
dbGetByName (char *songname)
{
  song_t *song = slistGetData (musicdb->songs, songname);
  return song;
}

song_t *
dbGetByIdx (dbidx_t idx)
{
  song_t  *song;

  if (idx < 0) {
    return NULL;
  }

  song = slistGetDataByIdx (musicdb->songs, idx);
  return song;
}

void
dbStartIterator (slistidx_t *iteridx)
{
  slistStartIterator (musicdb->songs, iteridx);
}

song_t *
dbIterate (dbidx_t *idx, slistidx_t *iteridx)
{
  song_t    *song;

  song = slistIterateValueData (musicdb->songs, iteridx);
  *idx = slistIterateGetIdx (musicdb->songs, iteridx);
  return song;
}

nlist_t *
dbGetDanceCounts (void)
{
  return musicdb->danceCounts;
}
