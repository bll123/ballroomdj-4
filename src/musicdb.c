#include "config.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "musicdb.h"
#include "rafile.h"
#include "song.h"
#include "list.h"
#include "bdjstring.h"
#include "lock.h"
#include "datafile.h"
#include "tagdef.h"
#include "log.h"

/* globals */
static int         initialized = 0;
static db_t        *bdjdb = NULL;

void
dbOpen (char *fn)
{
  if (! initialized) {
    bdjdb = malloc (sizeof (db_t));
    assert (bdjdb != NULL);
    bdjdb->songs = slistAlloc ("db-songs", LIST_UNORDERED,
        istringCompare, free, songFree);
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
  long        srrn;
  int         rc;

  fstr = "";
  radb = raOpen (fn, 10);
  slistSetSize (db->songs, raGetCount (radb));

  raStartBatch (radb);

  for (size_t i = 1L; i <= radb->count; ++i) {
    rc = raRead (radb, i, data);
    if (rc != 1) {
      logMsg (LOG_DBG, LOG_LVL_1, "ERR: Unable to access rrn %zd", i);
    }
    if (rc == 0 || ! *data) {
      continue;
    }

    song = songAlloc ();
    songParse (song, data);
    fstr = songGetData (song, TAG_KEY_FILE);
    srrn = songGetLong (song, TAG_KEY_RRN);
    if ((long) i != srrn) {
      songSetLong (song, TAG_KEY_RRN, (long) i);
    }
    slistSetData (db->songs, fstr, song);
    ++db->count;
  }
  slistSort (db->songs);

  raEndBatch (radb);
  raClose (radb);
  return 0;
}
