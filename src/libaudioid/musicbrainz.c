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
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "webclient.h"

typedef struct audioidmb {
  webclient_t   *webclient;
  const char    *webresponse;
  size_t        webresplen;
  mstime_t      globalreqtimer;
  int           respcount;
} audioidmb_t;

typedef struct {
  int             flag;
  int             tagidx;
  const char      *xpath;
  const char      *attr;
} mbxpath_t;

enum {
  MB_SINGLE,
  MB_RELEASE,
  MB_SKIP,
};

enum {
  MB_TYPE_JOINPHRASE = TAG_KEY_MAX + 1,
  MB_TYPE_RELCOUNT = TAG_KEY_MAX + 2,
};

// relative to /metadata/recording/release-list
static const char *mbreleasexpath = "/release";
static mbxpath_t mbxpaths [] = {
  { MB_SINGLE,  TAG_TITLE, "/metadata/recording/title", NULL },
  { MB_SINGLE,  TAG_DURATION, "/metadata/recording/length", NULL },
  { MB_SINGLE,  TAG_WORK_ID, "/metadata/recording/relation-list/relation/target", NULL },
  // joinphrase must appear before artist
  { MB_SKIP,    MB_TYPE_JOINPHRASE, "/metadata/recording/artist-credit/name-credit", "joinphrase" },
  { MB_SINGLE,  TAG_ARTIST, "/metadata/recording/artist-credit/name-credit/artist/name", NULL },
  // relcount must appear before the release xpaths
  { MB_SKIP,    MB_TYPE_RELCOUNT, "/metadata/recording/release-list", "count" },
  // relative to /metadata/recording/release-list/release
  { MB_RELEASE, TAG_ALBUM, "/title", NULL },
  { MB_RELEASE, TAG_DATE, "/date", NULL },
  { MB_RELEASE, TAG_ALBUMARTIST, "/artist-credit/name-credit/artist/name", NULL },
  { MB_RELEASE, TAG_DISCNUMBER, "/medium-list/medium/position", NULL },
  { MB_RELEASE, TAG_TRACKNUMBER, "/medium-list/medium/track-list/track/position", NULL },
  { MB_RELEASE, TAG_TRACKTOTAL, "/medium-list/medium/track-list", "count" },
};
enum {
  mbxpathcount = sizeof (mbxpaths) / sizeof (mbxpath_t),
};

static bool mbParse (audioidmb_t *mb, xmlXPathContextPtr xpathCtx, int xpathidx, int respidx, ilist_t *respdata);
static bool mbParseRelease (audioidmb_t *mb, xmlNodePtr cur, ilist_t *respdata);
static void mbWebResponseCallback (void *userdata, const char *resp, size_t len);

audioidmb_t *
mbInit (void)
{
  audioidmb_t *mb;

  mb = mdmalloc (sizeof (audioidmb_t));
  mb->webclient = webclientAlloc (mb, mbWebResponseCallback);
  mb->webresponse = NULL;
  mb->webresplen = 0;
  mstimeset (&mb->globalreqtimer, 0);
  xmlInitParser ();

  return mb;
}

void
mbFree (audioidmb_t *mb)
{
  if (mb == NULL) {
    return;
  }

  xmlCleanupParser ();
  webclientClose (mb->webclient);
  mb->webclient = NULL;
  mb->webresponse = NULL;
  mb->webresplen = 0;
  mdfree (mb);
}

void
mbRecordingIdLookup (audioidmb_t *mb, const char *recid, ilist_t *respdata)
{
  char          uri [MAXPATHLEN];
  ilistidx_t    iteridx;
  ilistidx_t    key;

  /* musicbrainz prefers only one call per second */
  while (! mstimeCheck (&mb->globalreqtimer)) {
    mssleep (10);
  }
  mstimeset (&mb->globalreqtimer, 1000);

  strlcpy (uri, sysvarsGetStr (SV_AUDIOID_MUSICBRAINZ_URI), sizeof (uri));
  strlcat (uri, "/recording/", sizeof (uri));
  strlcat (uri, recid, sizeof (uri));
  /* artist-credits retrieves the additional artists for the song */
  /* media is needed to get the track number and track total */
  /* work-rels is needed to get the work-id */
  /* releases is used to get the list of releases for this recording */
  /*    a match can then possibly be made by album name/track number */
  /* releases is used to get the list of releases for this recording */
  strlcat (uri, "?inc=artist-credits+work-rels+releases+artists+media+isrcs", sizeof (uri));
  logMsg (LOG_DBG, LOG_AUDIO_ID, "audioid: mb: uri: %s", uri);

  webclientGet (mb->webclient, uri);
  if (mb->webresponse != NULL) {
    xmlDocPtr           doc;
    xmlXPathContextPtr  xpathCtx;
    char                *twebresp;
    char                *p;

    /* response is too long to log */
    /* use the uri and paste it into a web browser */

    /* libxml2 doesn't have any way to set the default namespace for xpath */
    /* which makes it a pain to use when a namespace is set */
    twebresp = mdmalloc (mb->webresplen);
    memcpy (twebresp, mb->webresponse, mb->webresplen);
    p = strstr (twebresp, "xmlns");
    if (p != NULL) {
      char    *pe;
      size_t  len;

      pe = strstr (p, ">");
      len = pe - p;
      memset (p, ' ', len);
    }

    doc = xmlParseMemory (twebresp, mb->webresplen);
    if (doc == NULL) {
      return;
    }

    mb->respcount = 0;

    xpathCtx = xmlXPathNewContext (doc);
    if (xpathCtx == NULL) {
      xmlFreeDoc (doc);
    }

    for (int i = 0; i < mbxpathcount; ++i) {
      if (mbxpaths [i].flag == MB_RELEASE) {
        continue;
      }
      mbParse (mb, xpathCtx, i, 0, respdata);
    }

    /* copy the static data to each album */
    ilistStartIterator (respdata, &iteridx);
    while ((key = ilistIterateKey (respdata, &iteridx)) >= 0) {
      if (key == 0) {
        continue;
      }

      for (int j = 0; j < mbxpathcount; ++j) {
        int     tagidx;

        if (mbxpaths [j].flag != MB_SINGLE) {
          continue;
        }

        tagidx = mbxpaths [j].tagidx;
        ilistSetStr (respdata, key, tagidx, ilistGetStr (respdata, 0, tagidx));
      }
    }

    xmlXPathFreeContext (xpathCtx);
    xmlFreeDoc (doc);
  }

  return;
}

/* internal routines */

static bool
mbParse (audioidmb_t *mb, xmlXPathContextPtr xpathCtx, int xpathidx,
    int respidx, ilist_t *respdata)
{
  xmlXPathObjectPtr   xpathObj;
  xmlNodeSetPtr       nodes;
  xmlNodePtr          cur = NULL;
  const xmlChar       *val = NULL;
  const char          *joinphrase = NULL;
  int                 ncount;
  char                *nval = NULL;
  size_t              nlen = 0;

  xpathObj = xmlXPathEvalExpression ((xmlChar *) mbxpaths [xpathidx].xpath, xpathCtx);
  if (xpathObj == NULL)  {
    logMsg (LOG_DBG, LOG_IMPORTANT, "mbParse: bad xpath expression");
    return false;
  }

  nodes = xpathObj->nodesetval;
  if (xmlXPathNodeSetIsEmpty (nodes)) {
    xmlXPathFreeObject (xpathObj);
    return false;
  }
  ncount = nodes->nodeNr;

  joinphrase = ilistGetStr (respdata, 0, MB_TYPE_JOINPHRASE);
  for (int i = 0; i < ncount; ++i)  {
    size_t    len;

    if (nodes->nodeTab [i]->type != XML_ELEMENT_NODE) {
      continue;
    }

    cur = nodes->nodeTab [i];
    val = xmlNodeGetContent (cur);
    if (mbxpaths [xpathidx].attr != NULL) {
      val = xmlGetProp (cur, (xmlChar *) mbxpaths [xpathidx].attr);
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
      if (mbxpaths [xpathidx].tagidx == MB_TYPE_RELCOUNT) {
        mb->respcount = atoi (nval);
        mbParseRelease (mb, cur, respdata);
      } else {
        logMsg (LOG_DBG, LOG_AUDIO_ID, "mb: raw: set %d %s %s\n", respidx, tagdefs [mbxpaths [xpathidx].tagidx].tag, nval);
        ilistSetStr (respdata, respidx, mbxpaths [xpathidx].tagidx, nval);
        mdfree (nval);
        nval = NULL;
      }
    }
  }
  if (joinphrase != NULL) {
    ilistSetStr (respdata, 0, MB_TYPE_JOINPHRASE, NULL);
  }
  if (joinphrase != NULL && nval != NULL) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "mb: raw: set %d %s %s\n", respidx, tagdefs [mbxpaths [xpathidx].tagidx].tag, nval);
    ilistSetStr (respdata, mb->respcount, mbxpaths [xpathidx].tagidx, nval);
    mdfree (nval);
    nval = NULL;
  }

  ilistSetDouble (respdata, respidx, TAG_AUDIOID_SCORE, 100.0);
  xmlXPathFreeObject (xpathObj);
  return true;
}

static bool
mbParseRelease (audioidmb_t *mb, xmlNodePtr relnode, ilist_t *respdata)
{
  xmlXPathContextPtr  xpathCtx;
  xmlXPathObjectPtr   xpathObj;
  xmlNodeSetPtr       nodes;

  xpathCtx = xmlXPathNewContext ((xmlDocPtr) relnode);
  if (xpathCtx == NULL) {
    return false;
  }

  xpathObj = xmlXPathEvalExpression ((xmlChar *) mbreleasexpath, xpathCtx);
  if (xpathObj == NULL)  {
    logMsg (LOG_DBG, LOG_IMPORTANT, "mbParse: bad xpath expression");
    return false;
  }

  nodes = xpathObj->nodesetval;
  if (xmlXPathNodeSetIsEmpty (nodes)) {
    xmlXPathFreeObject (xpathObj);
    return false;
  }
  if (nodes->nodeNr < mb->respcount) {
    /* musicbrainz limits the response count to 25 */
    mb->respcount = nodes->nodeNr;
  }

  for (int i = 0; i < mb->respcount; ++i)  {
    xmlXPathContextPtr  relpathCtx;

    if (nodes->nodeTab [i]->type != XML_ELEMENT_NODE) {
      continue;
    }

    relpathCtx = xmlXPathNewContext ((xmlDocPtr) nodes->nodeTab [i]);
    if (relpathCtx == NULL) {
      continue;
    }

    for (int j = 0; j < mbxpathcount; ++j) {
      if (mbxpaths [j].flag != MB_RELEASE) {
        continue;
      }

      mbParse (mb, relpathCtx, j, i, respdata);
    }

    xmlXPathFreeContext (relpathCtx);
  }

  xmlXPathFreeContext (xpathCtx);
  return true;
}

static void
mbWebResponseCallback (void *userdata, const char *resp, size_t len)
{
  audioidmb_t   *mb = userdata;

  mb->webresponse = resp;
  mb->webresplen = len;
  return;
}

