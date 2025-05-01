/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
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

#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "expimp.h"
#include "filedata.h"
#include "ilist.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "playlist.h"
#include "tagdef.h"
#include "webclient.h"
#include "xmlparse.h"

static const char *titlexpath =
    "/rss/channel/title";
static const char *linkxpath =
    "/rss/channel/link";
static const char *itemxpath =
    "/rss/channel/item/title|"
    "/rss/channel/item/enclosure";
static const xmlparseattr_t itemattr [] = {
    { "title", RSS_ITEM_TITLE, NULL },
    { "enclosure", RSS_ITEM_URI, "url" },
    { "enclosure", RSS_ITEM_DURATION, "length" },
    { "enclosure", RSS_ITEM_TYPE, "type" },
    { NULL, -1, NULL },
};

typedef struct
{
  const char    *webresponse;
  size_t        webresplen;
} rssdata_t;

static void rssWebResponseCallback (void *userdata, const char *respstr, size_t len);

nlist_t *
rssImport (const char *uri)
{
  xmlparse_t    *xmlparse;
  ilist_t       *tlist = NULL;
  nlist_t       *implist;
  webclient_t   *webclient;
  rssdata_t     rssdata;
  int           webrc;
  char          tbuff [MAXPATHLEN];


  webclient = webclientAlloc (&rssdata, rssWebResponseCallback);
  webrc = webclientGet (webclient, uri);
  if (webrc != WEB_OK) {
    return NULL;
  }

  implist = nlistAlloc ("rssimport", LIST_ORDERED, NULL);
  nlistSetSize (implist, RSS_MAX);

  /* the RSS data has xmlns prefixes defined */
  /* using the xmlns is required */
  xmlparse = xmlParseInitData (rssdata.webresponse, rssdata.webresplen,
      XMLPARSE_USENS);
  xmlParseGetItem (xmlparse, titlexpath, tbuff, sizeof (tbuff));
  nlistSetStr (implist, RSS_TITLE, tbuff);
  xmlParseGetItem (xmlparse, linkxpath, tbuff, sizeof (tbuff));
  nlistSetStr (implist, RSS_URI, tbuff);
  tlist = xmlParseGetList (xmlparse, itemxpath, itemattr);
  nlistSetList (implist, RSS_ITEMS, tlist);

  xmlParseFree (xmlparse);
  webclientClose (webclient);

  return implist;
}

/* internal routines */

static void
rssWebResponseCallback (void *userdata, const char *respstr, size_t len)
{
  rssdata_t    *rssdata = (rssdata_t *) userdata;

  rssdata->webresponse = respstr;
  rssdata->webresplen = len;
  return;
}
