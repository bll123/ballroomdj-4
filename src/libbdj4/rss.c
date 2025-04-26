/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 *
 * Example podcast RSS files:
 *    https://www.npr.org/podcasts/2037/music
 *
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

// #include "audiosrc.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "expimp.h"
#include "filedata.h"
#include "mdebug.h"
// #include "nlist.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "playlist.h"
#include "tagdef.h"

void
rssImport (const char *fname, char *plname, size_t plsz)
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
  int                 itemcount;
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
    goto rssImportExit;
  }

  xpathObj = xmlXPathEvalExpression ((xmlChar *) "/rss/channel/title", xpathCtx);
  mdextalloc (xpathObj);
  nodes = xpathObj->nodesetval;
  if (nodes->nodeNr == 0) {
    goto rssImportExit;
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

  xpathObj = xmlXPathEvalExpression ((xmlChar *) "/rss/channel/link", xpathCtx);
  mdextalloc (xpathObj);
  nodes = xpathObj->nodesetval;
  if (nodes->nodeNr == 0) {
    goto rssImportExit;
  }

  cur = nodes->nodeTab [0];
  xval = xmlNodeGetContent (cur);
  mdextalloc (xval);
  if (xval != NULL) {
    // stpecpy (plname, plname + plsz, (char *) xval);
    mdextfree (xval);
    xmlFree (xval);
  }
  mdextfree (xpathObj);
  xmlXPathFreeObject (xpathObj);

  /* /rss/channel/item/title */
  /* /rss/channel/item/link */
  xpathObj = xmlXPathEvalExpression ((xmlChar *) "/rss/channel/item", xpathCtx);
  mdextalloc (xpathObj);
  if (xpathObj == NULL)  {
    goto rssImportExit;
  }

  nodes = xpathObj->nodesetval;
  if (xmlXPathNodeSetIsEmpty (nodes)) {
    goto rssImportExit;
  }
  itemcount = nodes->nodeNr;

  list = nlistAlloc ("rssimport", LIST_UNORDERED, NULL);

  for (int i = 0; i < itemcount; ++i)  {
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

    mdextfree (xval);
    xmlFree (xval);
  }

rssImportExit:
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
