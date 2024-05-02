/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
//#include "bdjopt.h"
#include "bdjstring.h"
#include "fileop.h"
#include "expimp.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathdisp.h"
//#include "pathutil.h"
#include "song.h"
#include "tagdef.h"
//#include "sysvars.h"

void
xspfExport (musicdb_t *musicdb, nlist_t *list,
    const char *fname, const char *slname)
{
  FILE        *fh;
  nlistidx_t  iteridx;
  dbidx_t     dbidx;

  fh = fileopOpen (fname, "w");
  if (fh == NULL) {
    return;
  }

  fprintf (fh, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf (fh, "<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n");
  fprintf (fh, "  <title>%s</title>\n", slname);
  fprintf (fh, "  <trackList>\n");

  nlistStartIterator (list, &iteridx);
  while ((dbidx = nlistIterateKey (list, &iteridx)) >= 0) {
    song_t        *song;
    const char    *str;
    const char    *pfx;
    char          ffn [MAXPATHLEN];

    song = dbGetByIdx (musicdb, dbidx);

    if (song == NULL) {
      continue;
    }
    pfx = "";
    if (audiosrcGetType (songGetStr (song, TAG_URI)) == AUDIOSRC_TYPE_FILE) {
      pfx = "file://";
    }

    fprintf (fh, "    <track>\n");

    str = songGetStr (song, TAG_URI);
    audiosrcFullPath (str, ffn, sizeof (ffn), 0, NULL);
    pathDisplayPath (ffn, strlen (ffn));
    fprintf (fh, "      <location>%s%s</location>\n", pfx, ffn);

    fprintf (fh, "      <title>%s</title>\n", songGetStr (song, TAG_TITLE));

    str = songGetStr (song, TAG_ARTIST);
    if (str != NULL && *str) {
      fprintf (fh, "      <creator>%s</creator>\n", str);
    }

    str = songGetStr (song, TAG_ALBUM);
    if (str != NULL && *str) {
      fprintf (fh, "      <album>%s</album>\n", str);
    }

    fprintf (fh, "      <duration>%" PRId64 "</duration>\n",
        songGetNum (song, TAG_DURATION));

    fprintf (fh, "    </track>\n");
  }

  fprintf (fh, "  </trackList>\n");
  fprintf (fh, "</playlist>\n");

  mdextfclose (fh);
  fclose (fh);
}

nlist_t *
xspfImport (musicdb_t *musicdb, const char *fname, char *plname, size_t plsz)
{
  FILE        *fh;
  nlist_t     *list;
  const char  *p;
  song_t      *song;
  dbidx_t     dbidx;
  char        tbuff [MAXPATHLEN];

  fh = fileopOpen (fname, "r");
  if (fh == NULL) {
    return NULL;
  }

  list = nlistAlloc ("xspfimport", LIST_UNORDERED, NULL);

#if 0
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

    pathNormalizePath (tbuff, strlen (tbuff));
    if (! fileopFileExists (tbuff)) {
      continue;
    }

    p = audiosrcRelativePath (tbuff, 0);

    song = dbGetByName (musicdb, p);
    if (song != NULL) {
      dbidx = songGetNum (song, TAG_DBIDX);
      if (dbidx >= 0) {
        nlistSetNum (list, dbidx, 0);
      }
    }
  }
#endif

  mdextfclose (fh);
  fclose (fh);
  return list;
}
