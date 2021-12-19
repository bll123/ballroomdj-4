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
#include "parse.h"

/* globals */
int         initialized = 0;
db_t        *bdjdb = NULL;

void
dbOpen (char *fn)
{
  if (! initialized) {
    bdjdb = malloc (sizeof (db_t));
    assert (bdjdb != NULL);
    bdjdb->songs = vlistAlloc (LIST_UNORDERED, VALUE_DATA, istringCompare,
        free, songFree);
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

int
dbLoad (db_t *db, char *fn)
{
  char        data [RAFILE_REC_SIZE];
  char        *fstr;
  song_t      *song;
  rafile_t    *radb;
  parseinfo_t *pi;
  int         songDataCount;

  pi = parseInit ();
  fstr = "";
  radb = raOpen (fn, 10);
  vlistSetSize (db->songs, raGetCount (radb));

  for (size_t i = 1L; i <= radb->count; ++i) {
    raRead (radb, i, data);
    if (! *data) {
      continue;
    }

    songDataCount = parse (pi, data);
    song = songAlloc ();
    songSetAll (song, parseGetData (pi), songDataCount);
//    fstr = songGet (song, TAG_FILE);
    songSetNumeric (song, "rrn", (long) i);
    vlistSetData (db->songs, strdup (fstr), song);
    ++db->count;
  }

  parseFree (pi);
  return 0;
}
