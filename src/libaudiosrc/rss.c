/*
 * Copyright 2025-2026 Brad Lanam Pleasant Hill CA
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
#include "slist.h"
#include "tagdef.h"
#include "tmutil.h"
#include "webclient.h"
#include "xmlparse.h"

static const char *titlexpath =
    "/rss/channel/title";
static const char *linkxpath =
    "/rss/channel/link";
static const char *blddatexpath =
    "/rss/channel/lastBuildDate";
static const char *imageurixpath =
    "/rss/channel/image/url";
static const char *itemxpath =
    "/rss/channel/item/title|"
    "/rss/channel/item/pubDate|"
    "/rss/channel/item/itunes:duration|"
    "/rss/channel/item/enclosure";
static const xmlparseattr_t itemattr [] = {
    { "title", RSS_ITEM_TITLE, NULL },
    { "pubDate", RSS_ITEM_DATE, NULL },
    { "duration", RSS_ITEM_DURATION, NULL },
    { "enclosure", RSS_ITEM_URI, "url" },
    { "enclosure", RSS_ITEM_TYPE, "type" },
    { NULL, -1, NULL },
};

typedef struct
{
  const char    *webresponse;
  size_t        webresplen;
  time_t        webresptime;
} rssdata_t;

static void rssWebResponseCallback (void *userdata, const char *respstr, size_t len, time_t tm);


time_t
rssGetUpdateTime (const char *uri)
{
  webclient_t   *webclient;
  rssdata_t     rssdata;
  int           webrc;

  webclient = webclientAlloc (&rssdata, rssWebResponseCallback);
  webclientSetTimeout (webclient, 1000);
  webrc = webclientHead (webclient, uri);
  if (webrc != WEB_OK) {
    return 0;
  }
  webclientClose (webclient);

  return rssdata.webresptime;
}

nlist_t *
rssImport (const char *uri)
{
  xmlparse_t    *xmlparse;
  ilist_t       *itemlist = NULL;
  slist_t       *itemidx = NULL;
  nlist_t       *implist;
  webclient_t   *webclient;
  rssdata_t     rssdata;
  int           webrc;
  char          tbuff [BDJ4_PATH_MAX];
  ilistidx_t    iteridx;
  ilistidx_t    key;
  time_t        tmval;

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

  xmlParseGetItem (xmlparse, imageurixpath, tbuff, sizeof (tbuff));
  nlistSetStr (implist, RSS_IMAGE_URI, tbuff);

  xmlParseGetItem (xmlparse, blddatexpath, tbuff, sizeof (tbuff));
  tmval = tmutilStringToUTC (tbuff, "%a, %d %h %Y %T %z");
  nlistSetNum (implist, RSS_BUILD_DATE, tmval);

  itemlist = xmlParseGetList (xmlparse, itemxpath, itemattr);
  nlistSetList (implist, RSS_ITEMS, itemlist);
  nlistSetNum (implist, RSS_COUNT, ilistGetCount (itemlist));

  xmlParseFree (xmlparse);
  webclientClose (webclient);

  itemidx = slistAlloc ("rssitemidx", LIST_UNORDERED, NULL);
  slistSetSize (itemidx, ilistGetCount (itemlist));

  ilistStartIterator (itemlist, &iteridx);
  while ((key = ilistIterateKey (itemlist, &iteridx)) >= 0) {
    const char  *val;

    val = ilistGetStr (itemlist, key, RSS_ITEM_URI);
    slistSetNum (itemidx, val, key);
    val = ilistGetStr (itemlist, key, RSS_ITEM_DATE);
    /* <pubDate>Wed, 23 Apr 2025 17:00:00 +0000</pubDate> */
    tmval = tmutilStringToUTC (val, "%a, %d %h %Y %T %z");
    snprintf (tbuff, sizeof (tbuff), "%zd", (size_t) tmval);
    ilistSetStr (itemlist, key, RSS_ITEM_DATE, tbuff);
  }
  slistSort (itemidx);
  nlistSetList (implist, RSS_IDX, itemidx);

  return implist;
}

/* internal routines */

static void
rssWebResponseCallback (void *userdata, const char *respstr, size_t len, time_t tm)
{
  rssdata_t    *rssdata = (rssdata_t *) userdata;

  rssdata->webresponse = respstr;
  rssdata->webresplen = len;
  rssdata->webresptime = tm;
  return;
}
