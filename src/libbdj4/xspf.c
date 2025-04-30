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
#include "bdjstring.h"
#include "fileop.h"
#include "expimp.h"
#include "filedata.h"
#include "ilist.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "song.h"
#include "tagdef.h"
#include "xmlparse.h"

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
    pfx = audiosrcPrefix (songGetStr (song, TAG_URI));

    fprintf (fh, "    <track>\n");

    str = songGetStr (song, TAG_URI);
    audiosrcFullPath (str, ffn, sizeof (ffn), NULL, 0);
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
  xmlparse_t          *xmlparse;
  ilist_t             *tlist = NULL;
  nlist_t             *implist = NULL;
  ilistidx_t          iteridx;
  ilistidx_t          key;
  int                 trkcount;
  const char          *str;
  const char          *val;
  song_t              *song;
  dbidx_t             dbidx;
  char                tbuff [MAXPATHLEN];

  xmlparse = xmlParseInit (fname);
  xmlParseGetItem (xmlparse, "/playlist/title", plname, plsz);
  tlist = xmlParseGetList (xmlparse, "/playlist/trackList/track/location");
  trkcount = ilistGetCount (tlist);

  implist = nlistAlloc ("xspfimport", LIST_UNORDERED, NULL);

  ilistStartIterator (tlist, &iteridx);
  while ((key = ilistIterateKey (tlist, &iteridx)) >= 0) {
    str = ilistGetStr (tlist, key, XMLPARSE_VAL);
    stpecpy (tbuff, tbuff + sizeof (tbuff), str);
    pathNormalizePath (tbuff, strlen (tbuff));
    val = audiosrcRelativePath (tbuff, 0);

    song = dbGetByName (musicdb, val);
    if (song != NULL) {
      dbidx = songGetNum (song, TAG_DBIDX);
      if (dbidx >= 0) {
        nlistSetNum (implist, dbidx, 0);
      }
    }
  }
  xmlParseFree (xmlparse);
  ilistFree (tlist);

  return implist;
}
