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

static int audioidParse (xmlXPathContextPtr xpathCtx, audioidxpath_t *xpaths, int xpathidx, int respidx, ilist_t *respdata, int level);
static int audioidParseTree (xmlNodeSetPtr nodes, audioidxpath_t *xpaths, int parenttagidx, int respidx, ilist_t *respdata, int level);

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
  audioidParse (xpathCtx, xpaths, 0, 0, respdata, 0);

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
    int xpathidx, int respidx, ilist_t *respdata, int level)
{
  xmlXPathObjectPtr   xpathObj;
  xmlNodeSetPtr       nodes;
  xmlNodePtr          cur = NULL;
  const xmlChar       *val = NULL;
  const char          *joinphrase = NULL;
  int                 ncount;
  char                *nval = NULL;
  size_t              nlen = 0;

fprintf (stderr, "%*s %d %s\n", level*2, "", xpathidx, xpaths [xpathidx].xpath);
  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s %d %s", level*2, "", xpathidx, xpaths [xpathidx].xpath);

  xpathObj = xmlXPathEvalExpression ((xmlChar *) xpaths [xpathidx].xpath, xpathCtx);
  if (xpathObj == NULL)  {
    logMsg (LOG_DBG, LOG_IMPORTANT, "audioidParse: bad xpath expression");
    return false;
  }

  nodes = xpathObj->nodesetval;
fprintf (stderr, "%*s node-count: %d\n", level*2, "", nodes->nodeNr);
  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s node-count: %d", level*2, "", nodes->nodeNr);
  if (xmlXPathNodeSetIsEmpty (nodes)) {
    xmlXPathFreeObject (xpathObj);
    return false;
  }
  ncount = nodes->nodeNr;

  if (xpaths [xpathidx].flag == AUDIOID_XPATH_TREE) {
fprintf (stderr, "%*s tree: xidx: %d %s\n", level*2, "", xpathidx, xpaths [xpathidx].xpath);
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: xidx: %d %s", level*2, "", xpathidx, xpaths [xpathidx].xpath);
    if (xpaths [xpathidx].tagidx == AUDIOID_TYPE_RESPONSE) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s response-count: %d", level*2, "", ncount);
    }
    audioidParseTree (nodes, xpaths [xpathidx].tree, xpaths [xpathidx].tagidx, respidx, respdata, level);
    return true;
  }

  for (int i = 0; i < ncount; ++i)  {
    size_t    len;

    if (nodes->nodeTab [i]->type != XML_ELEMENT_NODE) {
      continue;
    }

    cur = nodes->nodeTab [i];
    val = xmlNodeGetContent (cur);
    if (xpaths [xpathidx].attr != NULL) {
      val = xmlGetProp (cur, (xmlChar *) xpaths [xpathidx].attr);
    }
    if (val == NULL) {
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
fprintf (stderr, "%*s set %d %s %s\n", level*2, "", respidx, tagdefs [xpaths [xpathidx].tagidx].tag, nval);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set %d %s %s", level*2, "", respidx, tagdefs [xpaths [xpathidx].tagidx].tag, nval);
      ilistSetStr (respdata, respidx, xpaths [xpathidx].tagidx, nval);
      mdfree (nval);
      nval = NULL;
    }
  }

  if (joinphrase != NULL && nval != NULL) {
fprintf (stderr, "%*s set %d %s %s\n", level*2, "", respidx, tagdefs [xpaths [xpathidx].tagidx].tag, nval);
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set %d %s %s", level*2, "", respidx, tagdefs [xpaths [xpathidx].tagidx].tag, nval);
    ilistSetStr (respdata, respidx, xpaths [xpathidx].tagidx, nval);
    mdfree (nval);
    nval = NULL;
  }

  ilistSetDouble (respdata, respidx, TAG_AUDIOID_SCORE, 100.0);
  xmlXPathFreeObject (xpathObj);
  return true;
}

static int
audioidParseTree (xmlNodeSetPtr nodes, audioidxpath_t *xpaths,
    int parenttagidx, int respidx, ilist_t *respdata, int level)
{
  int   xidx;

  for (int i = 0; i < nodes->nodeNr; ++i)  {
    xmlXPathContextPtr  relpathCtx;

fprintf (stderr, "%*s tree: node: %d\n", level*2, "", i);
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: node: %d", level*2, "", i);
    if (nodes->nodeTab [i]->type != XML_ELEMENT_NODE) {
      continue;
    }

    relpathCtx = xmlXPathNewContext ((xmlDocPtr) nodes->nodeTab [i]);
    if (relpathCtx == NULL) {
      continue;
    }

    /* this is the containing tagidx */
    if (parenttagidx == AUDIOID_TYPE_RESPONSE) {
      respidx = i;
    }
fprintf (stderr, "%*s tree: respidx: %d\n", level*2, "", respidx);
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: respidx: %d", level*2, "", respidx);

    xidx = 0;
    while (xpaths [xidx].flag != AUDIOID_XPATH_END) {
      audioidParse (relpathCtx, xpaths, xidx, respidx, respdata, level + 1);
      ++xidx;
    }

    xmlXPathFreeContext (relpathCtx);
  }

fprintf (stderr, "%*s finish-tree %s\n", level*2, "", xpaths [xidx].xpath);
  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s finish-tree %s", level*2, "", xpaths [xidx].xpath);
  return true;
}
