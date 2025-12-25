/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "fileop.h"
#include "expimp.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "song.h"
#include "tagdef.h"
#include "sysvars.h"

void
m3uExport (musicdb_t *musicdb, nlist_t *list,
    const char *fname, const char *slname)
{
  FILE        *fh;
  nlistidx_t  iteridx;
  dbidx_t     dbidx;
  song_t      *song;
  char        tbuff [BDJ4_PATH_MAX];
  const char  *str;
  char        ffn [BDJ4_PATH_MAX];
  char        *tp = tbuff;
  char        *tend = tbuff + sizeof (tbuff);

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

    if (song == NULL) {
      continue;
    }
    if (audiosrcGetType (songGetStr (song, TAG_URI)) != AUDIOSRC_TYPE_FILE) {
      continue;
    }

    *tbuff = '\0';
    str = songGetStr (song, TAG_ARTIST);
    if (str != NULL && *str) {
      snprintf (tbuff, sizeof (tbuff), "%s - ", str);
    }
    tp = tbuff + strlen (tbuff);
    tp = stpecpy (tp, tend, songGetStr (song, TAG_TITLE));
    fprintf (fh, "#EXTINF:%" PRId64 ",%s\n",
        songGetNum (song, TAG_DURATION) / 1000, tbuff);
    str = songGetStr (song, TAG_ALBUMARTIST);
    if (str != NULL && *str) {
      fprintf (fh, "#EXTART:%s\n", str);
    }
    str = songGetStr (song, TAG_URI);
    audiosrcFullPath (str, ffn, sizeof (ffn), NULL, 0);
    pathDisplayPath (ffn, strlen (ffn));
    fprintf (fh, "%s\n", ffn);
  }
  mdextfclose (fh);
  fclose (fh);
}

nlist_t *
m3uImport (musicdb_t *musicdb, const char *fname)
{
  FILE        *fh;
  nlist_t     *list;
  const char  *p;
  song_t      *song;
  dbidx_t     dbidx;
  char        tbuff [BDJ4_PATH_MAX];

  fh = fileopOpen (fname, "r");
  if (fh == NULL) {
    return NULL;
  }

  list = nlistAlloc ("m3uimport", LIST_UNORDERED, NULL);

  while (fgets (tbuff, sizeof (tbuff), fh) != NULL) {
    if (strncmp (tbuff, "#PLAYLIST:", 10) == 0) {
      /* the playlist name is not used */
#if 0
      stringTrim (tbuff);
      p = tbuff + 10;
      while (*p == ' ' && *p != '\0') {
        ++p;
      }
      if (*p) {
        stpecpy (plname, plname + plsz, p);
      }
#endif
      continue;
    }

    if (*tbuff == '#') {
      continue;
    }

    stringTrim (tbuff);
    pathNormalizePath (tbuff, strlen (tbuff));
    p = audiosrcRelativePath (tbuff, 0);

    song = dbGetByName (musicdb, p);
    if (song != NULL) {
      dbidx = songGetNum (song, TAG_DBIDX);
      if (dbidx >= 0) {
        nlistSetNum (list, dbidx, 0);
      }
    }
  }

  mdextfclose (fh);
  fclose (fh);
  return list;
}
