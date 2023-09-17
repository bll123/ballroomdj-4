/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "audioid.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "tagdef.h"

static int audioidParse (xmlXPathContextPtr xpathCtx, audioidxpath_t *xpaths, int xpathidx, int respidx, ilist_t *respdata);
static int audioidParseTree (xmlNodeSetPtr nodes, audioidxpath_t *xpaths, int startxidx, int respcount, ilist_t *respdata);

void
audioidParseInit (void)
{
  xmlInitParser ();
}

void
audioidParseCleanup (void)
{
  xmlCleanupParser ();
}

int
audioidParseAll (const char *data, size_t datalen,
    audioidxpath_t *xpaths, ilist_t *respdata)
{
  xmlDocPtr           doc;
  xmlXPathContextPtr  xpathCtx;
  char                *tdata = NULL;
  char                *p;
  int                 xpathidx;
  int                 respcount;

  /* libxml2 doesn't have any way to set the default namespace for xpath */
  /* which makes it a pain to use when a namespace is set */
  tdata = mdmalloc (datalen);
  memcpy (tdata, data, datalen);
  p = strstr (tdata, "xmlns");
  if (p != NULL) {
    char    *pe;
    size_t  len;

    pe = strstr (p, ">");
    len = pe - p;
    memset (p, ' ', len);
  }

  doc = xmlParseMemory (tdata, datalen);
  if (doc == NULL) {
    return false;
  }

  respcount = 0;

  xpathCtx = xmlXPathNewContext (doc);
  if (xpathCtx == NULL) {
    xmlFreeDoc (doc);
  }

  /* the xpaths list must have a tree type in the beginning */
  respcount = audioidParse (xpathCtx, xpaths, 0, 0, respdata);

#if 0
  /* copy the static data to each album */
  ilistStartIterator (respdata, &iteridx);
  while ((key = ilistIterateKey (respdata, &iteridx)) >= 0) {
    int     xidx;

    if (key == 0) {
      continue;
    }

    xidx = 0;
    while (xpaths [xidx].flag != AUDIOID_XPATH_END) {
      int     tagidx;

      if (xpaths [xidx].flag != AUDIOID_XPATH_SINGLE) {
        ++xidx;
        continue;
      }

      tagidx = xpaths [xidx].tagidx;
      ilistSetStr (respdata, key, tagidx, ilistGetStr (respdata, 0, tagidx));
      ++xidx;
    }
  }
#endif

  xmlXPathFreeContext (xpathCtx);
  xmlFreeDoc (doc);
  mdfree (tdata);
  return true;
}

static int
audioidParse (xmlXPathContextPtr xpathCtx, audioidxpath_t *xpaths,
    int xpathidx, int respidx, ilist_t *respdata)
{
  xmlXPathObjectPtr   xpathObj;
  xmlNodeSetPtr       nodes;
  xmlNodePtr          cur = NULL;
  const xmlChar       *val = NULL;
  const char          *joinphrase = NULL;
  int                 ncount;
  char                *nval = NULL;
  size_t              nlen = 0;
  int                 respcount = 0;

  if (xpaths [xpathidx].flag == AUDIOID_XPATH_END_TREE ||
      xpaths [xpathidx].flag == AUDIOID_XPATH_END) {
fprintf (stderr, "   end/end-tree\n");
    return 0;
  }

fprintf (stderr, "-- %d %s\n", xpathidx, xpaths [xpathidx].xpath);

  xpathObj = xmlXPathEvalExpression ((xmlChar *) xpaths [xpathidx].xpath, xpathCtx);
  if (xpathObj == NULL)  {
fprintf (stderr, "   bad-xpath\n");
    logMsg (LOG_DBG, LOG_IMPORTANT, "audioidParse: bad xpath expression");
    return 0;
  }

  nodes = xpathObj->nodesetval;
fprintf (stderr, "   node-count: %d\n", nodes->nodeNr);
  if (xmlXPathNodeSetIsEmpty (nodes)) {
fprintf (stderr, "   no-nodes\n");
    xmlXPathFreeObject (xpathObj);
    return 0;
  }
  ncount = nodes->nodeNr;

  if (xpaths [xpathidx].flag == AUDIOID_XPATH_TREE) {
    respcount = audioidParseTree (nodes, xpaths, xpathidx, respcount, respdata);
fprintf (stderr, "   tree-parse-fin\n");
    return 1;
  }

  for (int i = 0; i < ncount; ++i)  {
    size_t    len;

    if (nodes->nodeTab [i]->type != XML_ELEMENT_NODE) {
      continue;
    }

    cur = nodes->nodeTab [i];
    val = xmlNodeGetContent (cur);
    if (val == NULL) {
fprintf (stderr, "   no-data\n");
      continue;
    }

    len = strlen ((const char *) val);
    if (joinphrase == NULL || i == 0) {
      nlen = len + 1;
      nval = mdmalloc (nlen);
      strlcpy (nval, (const char *) val, nlen);
    } else {
      nlen += len + strlen (joinphrase);
      nval = mdrealloc (nval, nlen);
      strlcat (nval, joinphrase, nlen);
      strlcat (nval, (const char *) val, nlen);
    }

    if (joinphrase == NULL && nval != NULL) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "raw: set %d %s %s\n", respidx, tagdefs [xpaths [xpathidx].tagidx].tag, nval);
fprintf (stderr, "   set %d %s %s\n", respidx, tagdefs [xpaths [xpathidx].tagidx].tag, nval);
      ilistSetStr (respdata, respidx, xpaths [xpathidx].tagidx, nval);
      mdfree (nval);
      nval = NULL;
    }
  }

  if (joinphrase != NULL && nval != NULL) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "raw: set %d %s %s\n", respidx, tagdefs [xpaths [xpathidx].tagidx].tag, nval);
fprintf (stderr, "   set %d %s %s\n", respidx, tagdefs [xpaths [xpathidx].tagidx].tag, nval);
    ilistSetStr (respdata, respcount, xpaths [xpathidx].tagidx, nval);
    mdfree (nval);
    nval = NULL;
  }

  ilistSetDouble (respdata, respidx, TAG_AUDIOID_SCORE, 100.0);
  xmlXPathFreeObject (xpathObj);
  return 0;
}

static int
audioidParseTree (xmlNodeSetPtr nodes, audioidxpath_t *xpaths,
    int startxidx, int respcount, ilist_t *respdata)
{
  xmlXPathContextPtr  xpathCtx;
  xmlXPathObjectPtr   xpathObj;
  int                 respidx = respcount;

fprintf (stderr, "-- tree: %d %s\n", startxidx, xpaths [startxidx].xpath);

fprintf (stderr, "      node-count: %d\n", nodes->nodeNr);
  if (xmlXPathNodeSetIsEmpty (nodes)) {
    xmlXPathFreeObject (xpathObj);
fprintf (stderr, "      empty-tree\n");
    return false;
  }
  if (xpaths [startxidx].tagidx == AUDIOID_TYPE_RESPONSE) {
    respcount = nodes->nodeNr;
fprintf (stderr, "      response-count: %d\n", respcount);
  }

  for (int i = 0; i < nodes->nodeNr; ++i)  {
    xmlXPathContextPtr  relpathCtx;
    int                 xidx;

fprintf (stderr, "   tree: %d %s\n", startxidx, xpaths [startxidx].xpath);
fprintf (stderr, "      tree: node %d\n", i);
    if (nodes->nodeTab [i]->type != XML_ELEMENT_NODE) {
      continue;
    }

    relpathCtx = xmlXPathNewContext ((xmlDocPtr) nodes->nodeTab [i]);
    if (relpathCtx == NULL) {
      continue;
    }

    if (xpaths [startxidx].tagidx == AUDIOID_TYPE_RESPONSE) {
      respidx = i;
    }

    xidx = startxidx + 1;
    while (xpaths [xidx].flag != AUDIOID_XPATH_END_TREE &&
        xpaths [xidx].flag != AUDIOID_XPATH_END) {
fprintf (stderr, "   tree: %d %s\n", startxidx, xpaths [startxidx].xpath);
fprintf (stderr, "      tree: parse %d xidx: %d respidx: %d\n", i, xidx, respidx);
      if (audioidParse (relpathCtx, xpaths, xidx, respidx, respdata)) {
fprintf (stderr, "      tree: parse-fin: startxidx: %d\n", startxidx);
        while (xpaths [xidx].flag != AUDIOID_XPATH_END_TREE &&
            xpaths [xidx].flag != AUDIOID_XPATH_END) {
          ++xidx;
        }
fprintf (stderr, "      tree: new xidx: %d (%d)\n", xidx, xidx+1);
      }
      ++xidx;
    }

    xmlXPathFreeContext (relpathCtx);
  }

  xmlXPathFreeContext (xpathCtx);
fprintf (stderr, "   -- finish-tree %d\n", startxidx);
  return respcount;
}
