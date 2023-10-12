/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 *
 * Parses XML responses
 *
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

static bool audioidParse (xmlXPathContextPtr xpathCtx, audioidparse_t *xpaths, int xpathidx, ilist_t *respdata, int level, audioid_id_t ident);
static bool audioidParseTree (xmlNodeSetPtr nodes, audioidparse_t *xpaths, int parenttagidx, ilist_t *respdata, int level, audioid_id_t ident);

void
audioidParseXMLInit (void)
{
  xmlInitParser ();
}

void
audioidParseXMLCleanup (void)
{
  xmlCleanupParser ();
}

int
audioidParseXMLAll (const char *data, size_t datalen,
    audioidparse_t *xpaths, ilist_t *respdata, audioid_id_t ident)
{
  xmlDocPtr           doc;
  xmlXPathContextPtr  xpathCtx;
  char                *tdata = NULL;
  char                *p;
  int                 respcount = 0;
  int                 respidx;

  /* libxml2 doesn't have any way to set the default namespace for xpath */
  /* which makes it a pain to use when a namespace is set */
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
  if (doc == NULL) {
    return 0;
  }

  xpathCtx = xmlXPathNewContext (doc);
  mdextalloc (xpathCtx);
  if (xpathCtx == NULL) {
    mdextfree (doc);
    xmlFreeDoc (doc);
    return 0;
  }

  /* beginning response index */
  respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);

  ilistSetNum (respdata, 0, AUDIOID_TYPE_JOINED, false);

  /* the xpaths list must have a tree type in the beginning */
  audioidParse (xpathCtx, xpaths, 0, respdata, 0, ident);

  respcount = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX) - respidx + 1;
  logMsg (LOG_DBG, LOG_AUDIO_ID, "parse: respcount: %d\n", respcount);

  mdextfree (xpathCtx);
  xmlXPathFreeContext (xpathCtx);
  mdextfree (doc);
  xmlFreeDoc (doc);
  mdfree (tdata);
  return respcount;
}

static bool
audioidParse (xmlXPathContextPtr xpathCtx, audioidparse_t *xpaths,
    int xpathidx, ilist_t *respdata, int level, audioid_id_t ident)
{
  xmlXPathObjectPtr   xpathObj;
  xmlNodeSetPtr       nodes;
  xmlNodePtr          cur = NULL;
  xmlChar             *val = NULL;
  const char          *joinphrase = NULL;
  int                 ncount;
  char                *nval = NULL;
  size_t              nlen = 0;
  int                 respidx;

  respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);
  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s xidx: %d %s respidx %d", level*2, "", xpathidx, xpaths [xpathidx].name, respidx);

  xpathObj = xmlXPathEvalExpression ((xmlChar *) xpaths [xpathidx].name, xpathCtx);
  mdextalloc (xpathObj);
  if (xpathObj == NULL)  {
    logMsg (LOG_DBG, LOG_IMPORTANT, "audioidParse: bad xpath expression");
    return false;
  }

  nodes = xpathObj->nodesetval;
  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s node-count: %d", level*2, "", nodes->nodeNr);
  if (xmlXPathNodeSetIsEmpty (nodes)) {
    mdextfree (xpathObj);
    xmlXPathFreeObject (xpathObj);
    return false;
  }
  ncount = nodes->nodeNr;

  if (xpaths [xpathidx].flag == AUDIOID_PARSE_TREE) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: xidx: %d %s", level*2, "", xpathidx, xpaths [xpathidx].name);
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
    audioidParseTree (nodes, xpaths [xpathidx].tree, xpaths [xpathidx].tagidx, respdata, level, ident);
    mdextfree (xpathObj);
    xmlXPathFreeObject (xpathObj);
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
    mdextalloc (val);
    if (xpaths [xpathidx].attr != NULL) {
      if (val != NULL) {
        mdextfree (val);
        xmlFree (val);
      }
      val = xmlGetProp (cur, (xmlChar *) xpaths [xpathidx].attr);
      mdextalloc (val);
    }
    if (val == NULL) {
      continue;
    }

    ttagidx = xpaths [xpathidx].tagidx;

    len = strlen ((const char *) val);
    joinphrase = ilistGetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE);
    nlen = len + 1;

    if (joinphrase != NULL) {
      nlen += strlen (joinphrase);
    }

    if ((ttagidx == TAG_ARTIST || ttagidx == TAG_ALBUMARTIST) &&
        ilistGetNum (respdata, 0, AUDIOID_TYPE_JOINED) == true) {
      oval = ilistGetStr (respdata, respidx, ttagidx);
      if (oval != NULL) {
        nlen += strlen (oval);
      }
    }

    nval = mdmalloc (nlen);
    *nval = '\0';

    if (oval != NULL &&
        ilistGetNum (respdata, 0, AUDIOID_TYPE_JOINED) == true) {
      strlcat (nval, oval, nlen);
      ilistSetNum (respdata, 0, AUDIOID_TYPE_JOINED, false);
    }

    strlcat (nval, (const char *) val, nlen);

    if (joinphrase != NULL) {
      strlcat (nval, joinphrase, nlen);
      ilistSetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE, NULL);
      ilistSetNum (respdata, 0, AUDIOID_TYPE_JOINED, true);
    }

    if (ttagidx == AUDIOID_TYPE_MONTH) {
      oval = ilistGetStr (respdata, respidx, TAG_DATE);
      if (oval != NULL) {
        char    tmp [40];
        int     tmonth;

        tmonth = atoi ((const char *) val);
        snprintf (tmp, sizeof (tmp), "%s-%02d", oval, tmonth);
        dataFree (nval);
        nval = mdstrdup (tmp);
        ttagidx = TAG_DATE;
      }
    }

    if (ttagidx < TAG_KEY_MAX) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set respidx: %d tagidx: %d %s %s", level*2, "", respidx, ttagidx, tagdefs [ttagidx].tag, nval);
    } else {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set respidx: %d tagidx: %d %s", level*2, "", respidx, ttagidx, nval);
    }
    if (ttagidx == TAG_AUDIOID_SCORE) {
      /* acoustid returns a score between 0 and 1.0 */
      ilistSetDouble (respdata, respidx, ttagidx, atof (nval) * 100.0);
    } else if (ttagidx == AUDIOID_TYPE_JOINPHRASE) {
      ilistSetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE, nval);
    } else {
      ilistSetStr (respdata, respidx, ttagidx, nval);
    }

    mdextfree (val);
    xmlFree (val);
    mdfree (nval);
    nval = NULL;
  }

  mdextfree (xpathObj);
  xmlXPathFreeObject (xpathObj);
  return true;
}

static bool
audioidParseTree (xmlNodeSetPtr nodes, audioidparse_t *xpaths,
    int parenttagidx, ilist_t *respdata, int level, audioid_id_t ident)
{
  int   xidx = 0;
  int   respidx;

  respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);

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

      ilistSetNum (respdata, respidx, AUDIOID_TYPE_IDENT, ident);
      if (ident == AUDIOID_ID_MB_LOOKUP) {
        ilistSetDouble (respdata, respidx, TAG_AUDIOID_SCORE, 100.0);
      }

      /* propagate values to the next response */
      if (respidx > 0) {
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: propagate from %d to %d", level*2, "", respidx - 1, respidx);
        dlist = ilistGetDatalist (respdata, respidx - 1);

        nlistStartIterator (dlist, &iteridx);
        while ((key = nlistIterateKey (dlist, &iteridx)) >= 0) {
          if (key == AUDIOID_TYPE_IDENT ||
              key == AUDIOID_TYPE_RESPIDX) {
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
    mdextalloc (relpathCtx);
    if (relpathCtx == NULL) {
      continue;
    }

    xidx = 0;
    while (xpaths [xidx].flag != AUDIOID_PARSE_END) {
      audioidParse (relpathCtx, xpaths, xidx, respdata, level + 1, ident);
      ++xidx;
    }

    /* increment the response index after the parse is done */
    if (parenttagidx == AUDIOID_TYPE_RESPIDX) {
      respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);
      ++respidx;
      ilistSetNum (respdata, 0, AUDIOID_TYPE_RESPIDX, respidx);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: set respidx: %d", level*2, "", respidx);
    }

    mdextfree (relpathCtx);
    xmlXPathFreeContext (relpathCtx);
  }

  ilistSetNum (respdata, 0, AUDIOID_TYPE_JOINED, false);

  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s finish-tree %s", level*2, "", xpaths [xidx].name);
  return true;
}
