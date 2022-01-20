#include "config.h"

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "datafile.h"
#include "nlist.h"
#include "slist.h"
#include "lock.h"
#include "log.h"
#include "musicdb.h"
#include "rafile.h"
#include "song.h"
#include "tagdef.h"

/* globals */
static int         initialized = 0;
static db_t        *bdjdb = NULL;

void
dbOpen (char *fn)
{
  if (! initialized) {
    dance_t       *dances;
    ssize_t       dcount;

    dances = bdjvarsdf [BDJVDF_DANCES];
    dcount = danceGetCount (dances);

    bdjdb = malloc (sizeof (db_t));
    assert (bdjdb != NULL);
    bdjdb->songs = slistAlloc ("db-songs", LIST_UNORDERED, free, songFree);
    bdjdb->danceCounts = NULL;
//    bdjdb->danceCounts = malloc (sizeof (ssize_t) * dcount);
    bdjdb->danceCount = dcount;
//    for (ssize_t i = 0; i < dcount; ++i) {
//      bdjdb->danceCounts [i] = 0;
//    }
    bdjdb->count = 0L;
    dbLoad (bdjdb, fn);
    initialized = 1;
  }
}

void
dbClose (void)
{
  /* for each song in db, free the song */
  if (bdjdb != NULL) {
    if (bdjdb->songs != NULL) {
      slistFree (bdjdb->songs);
    }
    free (bdjdb);
  }
  initialized = 0;
  bdjdb = NULL;
}

size_t
dbCount (void)
{
  size_t tcount = 0L;
  if (bdjdb != NULL) {
    tcount = bdjdb->count;
  }
  return tcount;
}

int
dbLoad (db_t *db, char *fn)
{
  char        data [RAFILE_REC_SIZE];
  char        *fstr;
  song_t      *song;
  rafile_t    *radb;
  rafileidx_t srrn;
  rafileidx_t rc;
  nlistidx_t  dkey;


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
    songParse (song, data);
    fstr = songGetData (song, TAG_FILE);
    srrn = songGetNum (song, TAG_RRN);
    dkey = songGetNum (song, TAG_DANCE);
//    if (dkey != LIST_VALUE_INVALID) {
//      db->danceCounts [dkey] += 1;
//    }
    if (i != srrn) {
      songSetNum (song, TAG_RRN, i);
    }
    slistSetData (db->songs, fstr, song);
    ++db->count;
  }
  slistSort (db->songs);

//  for (ssize_t i = 0; i < db->danceCount; ++i) {
//    logMsg (LOG_DBG, LOG_BASIC, "db-load: dance: %zd count: %zd", i, bdjdb->danceCounts [i]);
//  }

  raEndBatch (radb);
  raClose (radb);
  return 0;
}

song_t *
dbGetByName (char *songname)
{
  song_t *song = slistGetData (bdjdb->songs, songname);
  return song;
}

song_t *
dbGetByIdx (dbidx_t idx)
{
  song_t *song = slistGetDataByIdx (bdjdb->songs, idx);
  return song;
}

void
dbStartIterator (void)
{
  slistStartIterator (bdjdb->songs);
}

song_t *
dbIterate (dbidx_t *idx)
{
  song_t    *song;

  song = slistIterateValueData (bdjdb->songs);
  *idx = slistIterateGetIdx (bdjdb->songs);
  return song;
}

