/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
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
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "tagdef.h"

static bool audioidParse (xmlXPathContextPtr xpathCtx, audioidparse_t *xpaths, int xpathidx, audioid_resp_t *resp, int level, audioid_id_t ident);
static bool audioidParseTree (xmlNodeSetPtr nodes, audioidparse_t *xpaths, int parenttagidx, audioid_resp_t *resp, int level, audioid_id_t ident);

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
    audioidparse_t *xpaths, audioid_resp_t *resp, audioid_id_t ident)
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
  respidx = resp->respidx;

  /* the xpaths list must have a tree type in the beginning */
  audioidParse (xpathCtx, xpaths, 0, resp, 0, ident);

  respcount = resp->respidx - respidx + 1;
  logMsg (LOG_DBG, LOG_AUDIO_ID, "xml-parse: respcount: %d", respcount);

  mdextfree (xpathCtx);
  xmlXPathFreeContext (xpathCtx);
  mdextfree (doc);
  xmlFreeDoc (doc);
  mdfree (tdata);
  return respcount;
}

static bool
audioidParse (xmlXPathContextPtr xpathCtx, audioidparse_t *xpaths,
    int xpathidx, audioid_resp_t *resp, int level, audioid_id_t ident)
{
  xmlXPathObjectPtr   xpathObj;
  xmlNodeSetPtr       nodes;
  xmlNodePtr          cur = NULL;
  xmlChar             *val = NULL;
  int                 ncount;
  nlist_t             *respdata;

  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s xidx: %d %s respidx %d", level*2, "", xpathidx, xpaths [xpathidx].name, resp->respidx);

  xpathObj = xmlXPathEvalExpression ((xmlChar *) xpaths [xpathidx].name, xpathCtx);
  mdextalloc (xpathObj);
  if (xpathObj == NULL)  {
    logMsg (LOG_DBG, LOG_IMPORTANT, "xml-parse: bad xpath expression");
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

  respdata = audioidGetResponseData (resp, resp->respidx);

  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s parse: respidx: %d", level*2, "", resp->respidx);

  if (xpaths [xpathidx].flag == AUDIOID_PARSE_TREE) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: xidx: %d %s", level*2, "", xpathidx, xpaths [xpathidx].name);
    if (xpaths [xpathidx].tagidx == AUDIOID_TYPE_TOP) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s response-count: %d", level*2, "", ncount);
    }
    if (xpaths [xpathidx].tagidx == AUDIOID_TYPE_JOINPHRASE &&
        xpaths [xpathidx].attr != NULL) {
      cur = nodes->nodeTab [0];
      val = xmlGetProp (cur, (xmlChar *) xpaths [xpathidx].attr);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s store joinphrase(a) %s", level*2, "", (const char *) val);
      dataFree (resp->joinphrase);
      resp->joinphrase = NULL;
      if (val != NULL) {
        resp->joinphrase = mdstrdup ((const char *) val);
      }
    }
    audioidParseTree (nodes, xpaths [xpathidx].tree, xpaths [xpathidx].tagidx, resp, level, ident);

    mdextfree (xpathObj);
    xmlXPathFreeObject (xpathObj);
    return false;
  }

  for (int i = 0; i < ncount; ++i)  {
    const char  *tval = NULL;
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

    if (ttagidx == AUDIOID_TYPE_MONTH) {
      const char  *oval = NULL;

      oval = nlistGetStr (respdata, TAG_DATE);
      if (oval != NULL) {
        char    tmp [40];
        int     tmonth;

        tmonth = atoi ((const char *) val);
        snprintf (tmp, sizeof (tmp), "%s-%02d", oval, tmonth);

        ttagidx = TAG_DATE;
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set resp(%d): tagidx: %d %s %s", level*2, "", resp->respidx, ttagidx, tagdefs [ttagidx].tag, tmp);
        nlistSetStr (respdata, ttagidx, tmp);
        mdextfree (val);
        xmlFree (val);
        continue;
      }
    }

    tval = (const char *) val;

    if (ttagidx == AUDIOID_TYPE_ARTIST_TYPE) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s artist-type: %s", level*2, "", tval);
      if (strstr (tval, "conductor") != NULL) {
        resp->tagidx_add = TAG_CONDUCTOR;
      } else if (strstr (tval, "composer") != NULL) {
        resp->tagidx_add = TAG_COMPOSER;
      }
      mdextfree (val);
      xmlFree (val);
      continue;
    }

    if (ttagidx == TAG_AUDIOID_SCORE) {
      /* acoustid returns a score between 0 and 1.0 */
      /* this will need to be modified if other xml sources are implemented */
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set score(%d) %s", level*2, "", resp->respidx, tval);
      nlistSetDouble (respdata, ttagidx, atof (tval) * 100.0);
    } else if (ttagidx == AUDIOID_TYPE_JOINPHRASE) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s store joinphrase(b) %s", level*2, "", tval);
      dataFree (resp->joinphrase);
      resp->joinphrase = mdstrdup (tval);
    } else {
      audioidSetResponseData (level, resp, ttagidx, tval);
      if (resp->tagidx_add >= 0) {
        if (resp->tagidx_add == TAG_COMPOSER &&
            (ttagidx == TAG_SORT_ARTIST || ttagidx == TAG_SORT_ALBUMARTIST)) {
          resp->tagidx_add = TAG_SORT_COMPOSER;
        }
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set addtl %d/%s %s", level*2, "", resp->tagidx_add, tagdefs [resp->tagidx_add].tag, tval);
        audioidSetResponseData (level, resp, resp->tagidx_add, tval);
        resp->tagidx_add = -1;
      }
    }

    mdextfree (val);
    xmlFree (val);
  }

  mdextfree (xpathObj);
  xmlXPathFreeObject (xpathObj);
  return true;
}

static bool
audioidParseTree (xmlNodeSetPtr nodes, audioidparse_t *xpaths,
    int parenttagidx, audioid_resp_t *resp, int level, audioid_id_t ident)
{
  int       xidx = 0;
  nlist_t   *respdata;

  respdata = audioidGetResponseData (resp, resp->respidx);

  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: parse: respidx: %d", level*2, "", resp->respidx);

  for (int i = 0; i < nodes->nodeNr; ++i)  {
    xmlXPathContextPtr  relpathCtx;

    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: node: %d", level*2, "", i);
    if (nodes->nodeTab [i]->type != XML_ELEMENT_NODE) {
      continue;
    }

    relpathCtx = xmlXPathNewContext ((xmlDocPtr) nodes->nodeTab [i]);
    mdextalloc (relpathCtx);
    if (relpathCtx == NULL) {
      continue;
    }

    xidx = 0;
    while (xpaths [xidx].flag != AUDIOID_PARSE_END) {
      audioidParse (relpathCtx, xpaths, xidx, resp, level + 1, ident);
      ++xidx;
    }

    /* this is the tagidx containing the response */
    if (parenttagidx == AUDIOID_TYPE_TOP) {
      nlistidx_t    iteridx;
      nlistidx_t    key;
      const char    *tval;

      if (ident == AUDIOID_ID_MB_LOOKUP) {
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set score(%d) MB 100", level*2, "", resp->respidx);
        nlistSetDouble (respdata, TAG_AUDIOID_SCORE, 100.0);
      }

      if (logCheck (LOG_DBG, LOG_AUDIOID_DUMP)) {
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s -- resp b4-propagate (%d)", level*2, "", resp->respidx);
        audioidDumpResult (respdata);
      }

      /* propagate values to the next response */
      if (resp->respidx > 0) {
        nlist_t   *orespdata;

        logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: propagate from %d to %d", level*2, "", resp->respidx - 1, resp->respidx);

        orespdata = audioidGetResponseData (resp, resp->respidx - 1);
        if (logCheck (LOG_DBG, LOG_AUDIOID_DUMP)) {
          logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s -- old-resp (%d)", level*2, "", resp->respidx - 1);
          audioidDumpResult (orespdata);
        }

        nlistStartIterator (orespdata, &iteridx);
        while ((key = nlistIterateKey (orespdata, &iteridx)) >= 0) {
          if (key == TAG_AUDIOID_IDENT ||
              key == AUDIOID_TYPE_TOP ||
              key == AUDIOID_TYPE_RESPIDX) {
            continue;
          }
          if (key == TAG_AUDIOID_SCORE) {
            /* score is not propagated for mb-lookup */
            if (ident != AUDIOID_ID_MB_LOOKUP) {
              nlistSetDouble (respdata, key, nlistGetDouble (orespdata, key));
              logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s   p:score %.2f", level*2, "", nlistGetDouble (respdata, key));
            }
            continue;
          }

          /* propagate all data that is not already set */
          tval = nlistGetStr (respdata, key);
          if (tval == NULL || ! *tval) {
            nlistSetStr (respdata, key, nlistGetStr (orespdata, key));
            if (key < TAG_KEY_MAX) {
              logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s   p:%d/%s %s", level*2, "", key, tagdefs [key].tag, nlistGetStr (respdata, key));
            } else {
              logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s   p:%d %s", level*2, "", key, nlistGetStr (respdata, key));
            }
          }
        }
      }

      /* increment the response index after the parse is done */

      nlistSetNum (respdata, TAG_AUDIOID_IDENT, ident);
      resp->respidx += 1;

      if (logCheck (LOG_DBG, LOG_AUDIOID_DUMP)) {
        logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s -- parse-done: tree: new respidx: %d", level*2, "", resp->respidx);
        audioidDumpResult (respdata);
      }

      respdata = audioidGetResponseData (resp, resp->respidx);
    }

    mdextfree (relpathCtx);
    xmlXPathFreeContext (relpathCtx);
  }

  /* clear any join phrase once the parse of the tree is done */
  dataFree (resp->joinphrase);
  resp->joinphrase = NULL;

  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s finish-tree %s", level*2, "", xpaths [xidx].name);
  return true;
}
