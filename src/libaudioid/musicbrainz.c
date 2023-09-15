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
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
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
} audioidmb_t;

typedef struct {
  int             processidx;
  int             tagidx;
  const char      *xpath;
  const char      *attr;
  int             doprocess;
} mbxmlinfo_t;

enum {
  MB_TYPE_JOINPHRASE = TAG_KEY_MAX + 1,
  MB_TYPE_COUNT = TAG_KEY_MAX + 2,
};

static mbxmlinfo_t mbxmlinfo [] = {
  { 0, TAG_TITLE, "/metadata/recording/title", NULL, -1 },
  { 0, TAG_WORK_ID, "/metadata/recording/relation-list/relation/target", NULL, -1 },
  { 0, MB_TYPE_JOINPHRASE, "/metadata/recording/artist-credit/name-credit", "joinphrase", -1 },
  { 0, TAG_ARTIST, "/metadata/recording/artist-credit/name-credit/artist/name", NULL, -1 },
  { 0, MB_TYPE_COUNT, "/metadata/recording/release-list", "count", 1 },
  { 1, TAG_ALBUM, "/metadata/recording/release-list/release/title", NULL, -1 },
  { 1, TAG_DATE, "/metadata/recording/release-list/release/date", NULL, -1 },
  { 1, TAG_ALBUMARTIST, "/metadata/recording/release-list/release/artist-credit/name-credit/artist/name", NULL, -1 },
  { 1, TAG_DISCNUMBER, "/metadata/recording/release-list/release/medium-list/medium/position", NULL, -1 },
  { 1, TAG_TRACKNUMBER, "/metadata/recording/release-list/release/medium-list/medium/track-list/track/position", NULL, -1 },
  { 1, TAG_TRACKTOTAL, "/metadata/recording/release-list/release/medium-list/medium/track-list", "count", -1 },
};
enum {
  mbxmlinfocount = sizeof (mbxmlinfo) / sizeof (mbxmlinfo_t),
};


static bool mbParseXPath (xmlXPathContextPtr xpathCtx, int idx, nlist_t *rawdata);
static void mbWebResponseCallback (void *userdata, const char *resp, size_t len);
static void print_element_names (xmlNode * a_node, int level);

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

nlist_t *
mbRecordingIdLookup (audioidmb_t *mb, const char *recid)
{
  char    uri [MAXPATHLEN];
  nlist_t *resp = NULL;
  nlist_t *rawdata = NULL;

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
fprintf (stderr, "uri: %s\n", uri);

  webclientGet (mb->webclient, uri);
  if (mb->webresponse != NULL) {
    xmlDocPtr           doc;
    xmlXPathContextPtr  xpathCtx;
    char                *twebresp;
    char                *p;

// fprintf (stderr, "webresp:\n%.*s", (int) mb->webresplen, mb->webresponse);

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
      return resp;
    }

#if 0
{
xmlNode *root_element = NULL;
root_element = xmlDocGetRootElement (doc);
print_element_names (root_element, 0);
}
#endif

    rawdata = nlistAlloc ("mb-parse-tmp", LIST_ORDERED, NULL);

    xpathCtx = xmlXPathNewContext (doc);
    if (xpathCtx == NULL) {
      xmlFreeDoc (doc);
    }

    for (int i = 0; i < mbxmlinfocount; ++i) {
      mbParseXPath (xpathCtx, i, rawdata);
    }

    xmlXPathFreeContext (xpathCtx);
    xmlFreeDoc (doc);
  }

  return resp;
}

/* internal routines */

static bool
mbParseXPath (xmlXPathContextPtr xpathCtx, int idx, nlist_t *rawdata)
{
  xmlXPathObjectPtr   xpathObj;
  xmlNodeSetPtr       nodes;
  xmlNodePtr          cur;
  const xmlChar       *val = NULL;
  const char          *joinphrase = NULL;
  int                 ncount;
  char                *nval = NULL;
  size_t              nlen = 0;

  xpathObj = xmlXPathEvalExpression ((xmlChar *) mbxmlinfo [idx].xpath, xpathCtx);
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

  joinphrase = nlistGetStr (rawdata, MB_TYPE_JOINPHRASE);
  for (int i = 0; i < ncount; ++i)  {
    if (nodes->nodeTab [i]->type == XML_ELEMENT_NODE)  {
      size_t    len;

      cur = nodes->nodeTab [i];
      val = xmlNodeGetContent (cur);
      if (mbxmlinfo [idx].attr != NULL) {
        val = xmlGetProp (cur, (xmlChar *) mbxmlinfo [idx].attr);
      }
      if (val == NULL) {
        continue;
      }
fprintf (stderr, "path: %s\n   val: %s\n", mbxmlinfo [idx].xpath, (char *) val);
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
    }
  }
  if (joinphrase != NULL) {
    nlistSetStr (rawdata, MB_TYPE_JOINPHRASE, NULL);
  }
  if (nval != NULL) {
fprintf (stderr, "set %d %s\n", mbxmlinfo [idx].tagidx, nval);
    nlistSetStr (rawdata, mbxmlinfo [idx].tagidx, nval);
    mdfree (nval);
  }

  xmlXPathFreeObject (xpathObj);
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

static void
print_element_names (xmlNode * a_node, int level)
{
  const xmlNode *cur = NULL;
  const xmlChar *val = NULL;
  const xmlChar *count = NULL;

  for (cur = a_node; cur; cur = cur->next) {
    if (cur->type == XML_ELEMENT_NODE) {
      val = xmlNodeGetContent (cur);
      count = (xmlChar *) "";
      if (strcmp ((const char *) cur->name, "track-list") == 0 ||
          strcmp ((const char *) cur->name, "medium-list") == 0) {
        count = xmlGetProp (cur, (xmlChar *) "count");
      }
      fprintf (stderr, "%*s%s %s %s\n", level, "", cur->name, val, count);
    }
    print_element_names (cur->children, level + 1);
  }
}
