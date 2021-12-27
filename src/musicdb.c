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

/* globals */
static int         initialized = 0;
static db_t        *bdjdb = NULL;

void
dbOpen (char *fn)
{
  if (! initialized) {
    bdjdb = malloc (sizeof (db_t));
    assert (bdjdb != NULL);
    bdjdb->songs = vlistAlloc (KEY_STR, LIST_UNORDERED, istringCompare,
        free, songFree);
    bdjdb->count = 0L;
    dbLoad (bdjdb, fn);
    vlistSort (bdjdb->songs);
    initialized = 1;
  }
}

void
dbClose (void)
{
  /* for each song in db, free the song */
  if (bdjdb != NULL) {
    if (bdjdb->songs != NULL) {
      vlistFree (bdjdb->songs);
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
  parseinfo_t *pi;
  size_t      songDataCount;
  long        srrn;
  listkey_t   lkey;

  pi = parseInit ();
  fstr = "";
  radb = raOpen (fn, 10);
  vlistSetSize (db->songs, raGetCount (radb));

  raStartBatch (radb);

  for (size_t i = 1L; i <= radb->count; ++i) {
    raRead (radb, i, data);
    if (! *data) {
      continue;
    }

    songDataCount = parseKeyValue (pi, data);
    song = songAlloc ();
    songSetAll (song, parseGetData (pi), songDataCount);
    fstr = songGet (song, TAG_KEY_FILE);
    srrn = songGetLong (song, TAG_KEY_rrn);
    if ((long) i != srrn) {
      songSetLong (song, TAG_KEY_rrn, (long) i);
    }
    lkey.name = strdup (fstr);
    vlistSetData (db->songs, lkey, song);
    ++db->count;
  }

  raEndBatch (radb);
  raClose (radb);
  parseFree (pi);
  return 0;
}
