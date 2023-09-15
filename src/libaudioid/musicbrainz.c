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
  tagdefkey_t     tagidx;
  const char      *xpath;
  const char      *attr;
} mbxmlinfo_t;

static mbxmlinfo_t mbxmlinfo [] = {
  { TAG_TITLE, "/metadata/recording/title", NULL },
  { TAG_ARTIST, "/metadata/recording/artist-credit/name-credit/name", NULL },
  { TAG_ALBUM, "/metadata/recording/release-list/release/title", NULL },
  { TAG_DATE, "/metadata/recording/release-list/release/date", NULL },
  { TAG_ALBUMARTIST, "/metadata/recording/release-list/release/artist-credit/name-credit/artist/name", NULL },
  { TAG_DISCNUMBER, "/metadata/recording/release-list/release/medium-list/medium/position", NULL },
  { TAG_TRACKNUMBER, "/metadata/recording/release-list/release/medium-list/medium/track-list/track/position", NULL },
  // disctotal does not exist
  { TAG_TRACKTOTAL, "/metadata/recording/release-list/release/medium-list/medium/track-list", "count" },
  { TAG_WORK_ID, "/metadata/recording/relation-list/relation/target", NULL },
};

static bool mbParseXPath (xmlXPathContextPtr xpathCtx, const xmlChar *xpathExpr, slist_t *rawdata);
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

fprintf (stderr, "webresp:\n%.*s", (int) mb->webresplen, mb->webresponse);

    doc = xmlParseMemory (mb->webresponse, mb->webresplen);
    if (doc == NULL) {
      return resp;
    }

{
xmlNode *root_element = NULL;
root_element = xmlDocGetRootElement (doc);
print_element_names (root_element, 0);
}

    xpathCtx = xmlXPathNewContext (doc);
    if (xpathCtx == NULL) {
      xmlFreeDoc (doc);
    }

    xmlXPathFreeContext (xpathCtx);
    xmlFreeDoc (doc);
  }

  return resp;
}

/* internal routines */

static bool
mbParseXPath (xmlXPathContextPtr xpathCtx, const xmlChar *xpathExpr,
    slist_t *rawdata)
{
  xmlXPathObjectPtr   xpathObj;
//  xmlNodeSetPtr       nodes;
//  xmlNodePtr          cur;
//  xmlChar             *val = NULL;

  xpathObj = xmlXPathEvalExpression (xpathExpr, xpathCtx);
  if (xpathObj == NULL)  {
    logMsg (LOG_DBG, LOG_IMPORTANT, "mbParse: bad xpath expression");
    return false;
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
