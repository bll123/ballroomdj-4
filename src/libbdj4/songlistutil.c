/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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
  songlist_t  *tsl;
  song_t      *song;
  nlistidx_t  iteridx;
  ilistidx_t  key;
  dbidx_t     dbidx;
  int         distvers;

  songlist = songlistAlloc (fname);

  nlistStartIterator (dbidxlist, &iteridx);
  key = 0;
  while ((dbidx = nlistIterateKey (dbidxlist, &iteridx)) >= 0) {
    song = dbGetByIdx (musicdb, dbidx);

    songlistSetStr (songlist, key, SONGLIST_FILE, songGetStr (song, TAG_FILE));
    songlistSetStr (songlist, key, SONGLIST_TITLE, songGetStr (song, TAG_TITLE));
    songlistSetNum (songlist, key, SONGLIST_DANCE, songGetNum (song, TAG_DANCE));
    ++key;
  }

  tsl = songlistLoad (fname);
  distvers = songlistDistVersion (tsl);
  songlistFree (tsl);
  songlistSave (songlist, SONGLIST_UPDATE_TIMESTAMP, distvers);
  songlistFree (songlist);
}