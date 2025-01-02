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

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "expimp.h"
#include "filedata.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "song.h"
#include "tagdef.h"

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
  xmlDocPtr           doc = NULL;
  xmlXPathContextPtr  xpathCtx = NULL;
  xmlXPathObjectPtr   xpathObj = NULL;
  xmlNodeSetPtr       nodes = NULL;
  xmlNodePtr          cur = NULL;
  char                *data = NULL;
  char                *tdata = NULL;
  size_t              datalen = 0;
  char                *p;
  nlist_t             *list = NULL;
  int                 trkcount;
  xmlChar             *xval;
  const char          *val;
  song_t              *song;
  dbidx_t             dbidx;
  char                tbuff [MAXPATHLEN];

  data = filedataReadAll (fname, &datalen);
  if (data == NULL) {
    return NULL;
  }

  /* clear out the namespace */
  tdata = mdmalloc (datalen + 1);
  memcpy (tdata, data, datalen);
  tdata [datalen] = '\0';
  p = strstr (tdata, "xmlns");
  if (p != NULL) {
    char    *pe;
    size_t  len;

    pe = strstr (p, ">");
    len = pe - p;
    memset (p, ' ', len);
  }

  doc = xmlParseMemory (tdata, datalen);
  mdextalloc (doc);

  xpathCtx = xmlXPathNewContext (doc);
  mdextalloc (xpathCtx);
  if (xpathCtx == NULL) {
    goto xspfImportExit;
  }

  xpathObj = xmlXPathEvalExpression ((xmlChar *) "/playlist/title", xpathCtx);
  mdextalloc (xpathObj);
  nodes = xpathObj->nodesetval;
  if (nodes->nodeNr == 0) {
    goto xspfImportExit;
  }

  cur = nodes->nodeTab [0];
  xval = xmlNodeGetContent (cur);
  mdextalloc (xval);
  if (xval != NULL) {
    stpecpy (plname, plname + plsz, (char *) xval);
    mdextfree (xval);
    xmlFree (xval);
  }
  mdextfree (xpathObj);
  xmlXPathFreeObject (xpathObj);

  xpathObj = xmlXPathEvalExpression ((xmlChar *) "/playlist/trackList/track/location", xpathCtx);
  mdextalloc (xpathObj);
  if (xpathObj == NULL)  {
    goto xspfImportExit;
  }

  nodes = xpathObj->nodesetval;
  if (xmlXPathNodeSetIsEmpty (nodes)) {
    goto xspfImportExit;
  }
  trkcount = nodes->nodeNr;

  list = nlistAlloc ("xspfimport", LIST_UNORDERED, NULL);

  for (int i = 0; i < trkcount; ++i)  {
    if (nodes->nodeTab [i]->type != XML_ELEMENT_NODE) {
      continue;
    }

    cur = nodes->nodeTab [i];
    xval = xmlNodeGetContent (cur);
    if (xval == NULL) {
      continue;
    }
    mdextalloc (xval);

    stpecpy (tbuff, tbuff + sizeof (tbuff), (char *) xval);
    pathNormalizePath (tbuff, strlen (tbuff));
    val = audiosrcRelativePath (tbuff, 0);

    song = dbGetByName (musicdb, val);
    if (song != NULL) {
      dbidx = songGetNum (song, TAG_DBIDX);
      if (dbidx >= 0) {
        nlistSetNum (list, dbidx, 0);
      }
    }

    mdextfree (xval);
    xmlFree (xval);
  }

xspfImportExit:
  if (xpathObj != NULL) {
    mdextfree (xpathObj);
    xmlXPathFreeObject (xpathObj);
  }
  if (xpathCtx != NULL) {
    mdextfree (xpathCtx);
    xmlXPathFreeContext (xpathCtx);
  }
  if (doc != NULL) {
    mdextfree (doc);
    xmlFreeDoc (doc);
  }
  dataFree (tdata);
  dataFree (data);

  return list;
}
