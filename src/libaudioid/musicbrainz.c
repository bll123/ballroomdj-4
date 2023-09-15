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
#include "webclient.h"

typedef struct audioidmb {
  webclient_t   *webclient;
  const char    *webresponse;
  size_t        webresplen;
} audioidmb_t;

static const xmlChar * artistxpath = (const xmlChar *)
      "/metadata/recording/artist-credit/name-credit";

static bool mbParseXPath (xmlXPathContextPtr xpathCtx, const xmlChar *xpathExpr, slist_t *rawdata);
static void mbWebResponseCallback (void *userdata, const char *resp, size_t len);
static void print_element_names (xmlNode * a_node);

audioidmb_t *
mbInit (void)
{
  audioidmb_t *mb;

  mb = mdmalloc (sizeof (audioidmb_t));
  mb->webclient = webclientAlloc (mb, mbWebResponseCallback);
  mb->webresponse = NULL;
  mb->webresplen = 0;
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

  strlcpy (uri, sysvarsGetStr (SV_AUDIOID_MUSICBRAINZ_URI), sizeof (uri));
  strlcat (uri, recid, sizeof (uri));
  /* artist-credits retrieves the additional artists for the song */
  /* media is needed to get the track number and track total */
  /* work-rels is needed to get the work-id */
  /* releases is used to get the list of releases for this recording */
  /*    a match can then possibly be made by album name/track number */
  /* releases is used to get the list of releases for this recording */
  strlcat (uri, "?inc=artist-credits+work-rels+releases+artists+media", sizeof (uri));

  webclientGet (mb->webclient, uri);
  if (mb->webresponse != NULL) {
    xmlDocPtr           doc;
    xmlXPathContextPtr  xpathCtx;

    doc = xmlParseMemory (mb->webresponse, mb->webresplen);
    if (doc == NULL) {
      return resp;
    }

{
xmlNode *root_element = NULL;
root_element = xmlDocGetRootElement(doc);
print_element_names(root_element);
}

    xpathCtx = xmlXPathNewContext (doc);
    if (xpathCtx == NULL) {
      xmlFreeDoc (doc);
    }

    xmlXPathFreeContext (xpathCtx);
    xmlFreeDoc (doc);
fprintf (stderr, "resp:\n%.*s", (int) mb->webresplen, mb->webresponse);
  }

  return resp;
}

/* internal routines */

static bool
mbParseXPath (xmlXPathContextPtr xpathCtx, const xmlChar *xpathExpr,
    slist_t *rawdata)
{
  xmlXPathObjectPtr   xpathObj;
  xmlNodeSetPtr       nodes;
  xmlNodePtr          cur;
  xmlChar             *val = NULL;

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
print_element_names(xmlNode * a_node)
{
  xmlNode *cur_node = NULL;

  for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      printf ("node type: Element, name: %s\n", cur_node->name);
    }
    print_element_names(cur_node->children);
  }
}
