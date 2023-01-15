/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "fileop.h"
#include "m3u.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathutil.h"
#include "song.h"
#include "songutil.h"
#include "tagdef.h"
#include "sysvars.h"

void
m3uExport (musicdb_t *musicdb, nlist_t *list,
    const char *fname, const char *slname)
{
  FILE        *fh;
  nlistidx_t  iteridx;
  nlistidx_t  saveiteridx;
  dbidx_t     dbidx;
  song_t      *song;
  char        tbuff [MAXPATHLEN];
  char        *str;
  char        *ffn;

  fh = fileopOpen (fname, "w");
  if (fh == NULL) {
    return;
  }

  fprintf (fh, "#EXTM3U\n");
  fprintf (fh, "#EXTENC:UTF-8\n");
  fprintf (fh, "#PLAYLIST:%s\n", slname);

  nlistStartIterator (list, &iteridx);
  while ((dbidx = nlistIterateKey (list, &iteridx)) >= 0) {
    song = dbGetByIdx (musicdb, dbidx);

    *tbuff = '\0';
    str = songGetStr (song, TAG_ARTIST);
    if (str != NULL && *str) {
      snprintf (tbuff, sizeof (tbuff), "%s - ", str);
    }
    strlcat (tbuff, songGetStr (song, TAG_TITLE), sizeof (tbuff));
    fprintf (fh, "#EXTINF:%"PRId64",%s\n",
        songGetNum (song, TAG_DURATION) / 1000, tbuff);
    str = songGetStr (song, TAG_ALBUMARTIST);
    if (str != NULL && *str) {
      fprintf (fh, "#EXTART:%s\n", str);
    }
    ffn = nlistGetStr (list, dbidx);
    if (ffn == NULL) {
      str = songGetStr (song, TAG_FILE);
      ffn = songFullFileName (str);
    }
    if (isWindows ()) {
      pathWinPath (ffn, strlen (ffn));
    }
    fprintf (fh, "%s\n", ffn);
  }
}

nlist_t *
m3uImport (musicdb_t *musicdb, const char *fname, char *plname, size_t plsz)
{
  FILE        *fh;
  nlist_t     *list;
  char        *musicdir;
  size_t      musicdirlen;
  char        *p;
  song_t      *song;
  dbidx_t     dbidx;
  char        tbuff [MAXPATHLEN];


  fh = fileopOpen (fname, "r");
  if (fh == NULL) {
    return NULL;
  }

  list = nlistAlloc ("m3uimport", LIST_UNORDERED, NULL);
  musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
  musicdirlen = strlen (musicdir);

  while (fgets (tbuff, sizeof (tbuff), fh) != NULL) {
    if (strncmp (tbuff, "#PLAYLIST:", 10) == 0) {
      stringTrim (tbuff);
      p = tbuff + 10;
      while (*p == ' ' && *p != '\0') {
        ++p;
      }
      if (*p) {
        strlcpy (plname, p, plsz);
      }
      continue;
    }

    if (*tbuff == '#') {
      continue;
    }

    stringTrim (tbuff);

    if (! fileopFileExists (tbuff)) {
      continue;
    }

    p = tbuff;
    if (strncmp (p, musicdir, musicdirlen) == 0) {
      p += musicdirlen;
      p += 1;
    }

    song = dbGetByName (musicdb, p);
    dbidx = songGetNum (song, TAG_DBIDX);
    if (dbidx >= 0) {
      nlistSetNum (list, dbidx, 0);
    }
  }

  fclose (fh);
  return list;
}
