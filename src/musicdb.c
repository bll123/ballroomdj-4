#include "bdjconfig.h"

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
    bdjdb->songs = vlistAlloc (LIST_UNORDERED, istringCompare,
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
  vlistFree (bdjdb->songs);
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
    fstr = songGet (song, TAG_FILE);
    sscanf (songGet (song, "rrn"), "%ld", &srrn);
    if ((long) i != srrn) {
      songSetNumeric (song, "rrn", (long) i);
    }
    vlistSetData (db->songs, strdup (fstr), song);
    ++db->count;
  }

  logVarMsg (LOG_SESS, "dbLoad: database loaded: ", "%d", db->count);
  raEndBatch (radb);
  parseFree (pi);
  return 0;
}
