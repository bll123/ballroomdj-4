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

static bool audioidParse (xmlXPathContextPtr xpathCtx, audioidxpath_t *xpaths, int xpathidx, int respidx, ilist_t *respdata, int level);
static bool audioidParseTree (xmlNodeSetPtr nodes, audioidxpath_t *xpaths, int parenttagidx, int respidx, ilist_t *respdata, int level);

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
  int                 respcount = 0;
  int                 respidx;

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
    return 0;
  }

  xpathCtx = xmlXPathNewContext (doc);
  if (xpathCtx == NULL) {
    xmlFreeDoc (doc);
  }

  /* the xpaths list must have a tree type in the beginning */
  respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);
  audioidParse (xpathCtx, xpaths, 0, respidx, respdata, 0);
  respcount = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX) - respidx + 1;
  logMsg (LOG_DBG, LOG_AUDIO_ID, "parse: respcount: %d\n", respcount);

  xmlXPathFreeContext (xpathCtx);
  xmlFreeDoc (doc);
  mdfree (tdata);
  return respcount;
}

static bool
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

  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s %d %s", level*2, "", xpathidx, xpaths [xpathidx].xpath);

  xpathObj = xmlXPathEvalExpression ((xmlChar *) xpaths [xpathidx].xpath, xpathCtx);
  if (xpathObj == NULL)  {
    logMsg (LOG_DBG, LOG_IMPORTANT, "audioidParse: bad xpath expression");
    return false;
  }

  nodes = xpathObj->nodesetval;
  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s node-count: %d", level*2, "", nodes->nodeNr);
  if (xmlXPathNodeSetIsEmpty (nodes)) {
    xmlXPathFreeObject (xpathObj);
    return false;
  }
  ncount = nodes->nodeNr;

  if (xpaths [xpathidx].flag == AUDIOID_XPATH_TREE) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: xidx: %d %s", level*2, "", xpathidx, xpaths [xpathidx].xpath);
    if (xpaths [xpathidx].tagidx == AUDIOID_TYPE_RESPIDX) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s response-count: %d", level*2, "", ncount);
    }
    if (xpaths [xpathidx].tagidx == AUDIOID_TYPE_JOINPHRASE &&
        xpaths [xpathidx].attr != NULL) {
      cur = nodes->nodeTab [0];
      val = xmlGetProp (cur, (xmlChar *) xpaths [xpathidx].attr);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s store joinphrase %s", level*2, "", (const char *) val);
      ilistSetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE, (const char *) val);
    }
    audioidParseTree (nodes, xpaths [xpathidx].tree, xpaths [xpathidx].tagidx, respidx, respdata, level);
    return false;
  }

  for (int i = 0; i < ncount; ++i)  {
    size_t      len;
    const char  *oval = NULL;
    int         ttagidx;

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
    joinphrase = ilistGetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE);
    nlen = len + 1;
    if (joinphrase != NULL) {
      oval = ilistGetStr (respdata, respidx, xpaths [xpathidx].tagidx);
      if (oval != NULL) {
        nlen += strlen (oval) + strlen (joinphrase);
      }
    }
    nval = mdmalloc (nlen);
    *nval = '\0';

    if (joinphrase != NULL && oval != NULL) {
      strlcat (nval, oval, nlen);
      strlcat (nval, joinphrase, nlen);
      ilistSetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE, NULL);
    }

    strlcat (nval, (const char *) val, nlen);

    ttagidx = xpaths [xpathidx].tagidx;
    if (ttagidx == AUDIOID_TYPE_MONTH) {
      char    tmp [40];

      oval = ilistGetStr (respdata, respidx, TAG_DATE);
      if (oval != NULL) {
        snprintf (tmp, sizeof (tmp), "%s-%s", oval, (const char *) val);
        mdfree (nval);
        nval = mdstrdup (tmp);
        ttagidx = TAG_DATE;
      }
    }

    if (ttagidx < TAG_KEY_MAX) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set %d %s %s", level*2, "", respidx, tagdefs [ttagidx].tag, nval);
    } else {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set %d %s", level*2, "", respidx, nval);
    }
    if (ttagidx == TAG_AUDIOID_SCORE) {
      ilistSetDouble (respdata, respidx, ttagidx, atof (nval) * 100.0);
    } else {
      if (ttagidx == AUDIOID_TYPE_JOINPHRASE) {
        ilistSetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE, nval);
      } else {
        ilistSetStr (respdata, respidx, ttagidx, nval);
      }
    }
    mdfree (nval);
    nval = NULL;
  }

  xmlXPathFreeObject (xpathObj);
  return true;
}

static bool
audioidParseTree (xmlNodeSetPtr nodes, audioidxpath_t *xpaths,
    int parenttagidx, int respidx, ilist_t *respdata, int level)
{
  int   xidx = 0;

  for (int i = 0; i < nodes->nodeNr; ++i)  {
    xmlXPathContextPtr  relpathCtx;

    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: node: %d", level*2, "", i);
    if (nodes->nodeTab [i]->type != XML_ELEMENT_NODE) {
      continue;
    }

    /* this is the containing tagidx */
    if (parenttagidx == AUDIOID_TYPE_RESPIDX) {
      nlistidx_t    iteridx;
      nlistidx_t    key;
      nlist_t       *dlist;

      if (respidx > 0) {
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: propagate from %d to %d", level*2, "", respidx - 1, respidx);
        dlist = ilistGetDatalist (respdata, respidx - 1);

        nlistStartIterator (dlist, &iteridx);
        while ((key = nlistIterateKey (dlist, &iteridx)) >= 0) {
          if (key == AUDIOID_TYPE_RESPIDX) {
            continue;
          }
          if (key == TAG_AUDIOID_SCORE) {
            ilistSetDouble (respdata, respidx, key, nlistGetDouble (dlist, key));
            continue;
          }

          /* propagate all data */
          ilistSetStr (respdata, respidx, key, nlistGetStr (dlist, key));
        }
      }
    }

    relpathCtx = xmlXPathNewContext ((xmlDocPtr) nodes->nodeTab [i]);
    if (relpathCtx == NULL) {
      continue;
    }

    xidx = 0;
    while (xpaths [xidx].flag != AUDIOID_XPATH_END) {
      audioidParse (relpathCtx, xpaths, xidx, respidx, respdata, level + 1);
      ++xidx;
    }

    /* increment the response index after the parse is done */
    if (parenttagidx == AUDIOID_TYPE_RESPIDX) {
      respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);
      ++respidx;
      ilistSetNum (respdata, 0, AUDIOID_TYPE_RESPIDX, respidx);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: set respidx: %d", level*2, "", respidx);
    }

    xmlXPathFreeContext (relpathCtx);
  }

  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s finish-tree %s", level*2, "", xpaths [xidx].xpath);
  return true;
}
