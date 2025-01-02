/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "ilist.h"
#include "nlist.h"
#include "mdebug.h"
#include "songlist.h"
#include "songlistutil.h"
#include "tagdef.h"

void
songlistutilCreateFromList (musicdb_t *musicdb, const char *fname,
    nlist_t *dbidxlist)
{
  songlist_t  *songlist;
  song_t      *song;
  nlistidx_t  iteridx;
  ilistidx_t  key;
  dbidx_t     dbidx;
  int         distvers = 1;

  if (songlistExists (fname)) {
    songlist = songlistLoad (fname);
    distvers = songlistDistVersion (songlist);
    songlistClear (songlist);
  } else {
    songlist = songlistCreate (fname);
  }

  nlistStartIterator (dbidxlist, &iteridx);
  key = 0;
  while ((dbidx = nlistIterateKey (dbidxlist, &iteridx)) >= 0) {
    song = dbGetByIdx (musicdb, dbidx);

    if (song == NULL) {
      continue;
    }

    songlistSetStr (songlist, key, SONGLIST_URI, songGetStr (song, TAG_URI));
    songlistSetStr (songlist, key, SONGLIST_TITLE, songGetStr (song, TAG_TITLE));
    songlistSetNum (songlist, key, SONGLIST_DANCE, songGetNum (song, TAG_DANCE));
    ++key;
  }

  songlistSave (songlist, SONGLIST_UPDATE_TIMESTAMP, distvers);
  songlistFree (songlist);
}
