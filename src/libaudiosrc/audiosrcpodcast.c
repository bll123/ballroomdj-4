/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "expimp.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "ilist.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathinfo.h"
#include "playlist.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "webclient.h"

enum {
  ASPODCAST_WAIT_MAX = 200,
  ASPODCAST_CLIENT_MAX = 5,
};

enum {
  AS_CLIENT_URI,
  AS_CLIENT_URI_LEN,
  AS_CLIENT_MAX,
};

typedef struct asiterdata {
  slistidx_t    iteridx;
  slist_t       *iterlist;
  slist_t       *songlist;
  slist_t       *songtags;
  slist_t       *plNames;
} asiterdata_t;

typedef struct asdata {
  webclient_t   **webclient;
  ilist_t       *client;
  const char    *webresponse;
  size_t        webresplen;
  int           clientcount;
  int           state;
  nlist_t       *rssdata;
  time_t        rsslastbldtm;
} asdata_t;

static void aspodcastWebResponseCallback (void *userdata, const char *respstr, size_t len, time_t tm);
static bool aspodcastGetPlaylistNames (asdata_t *asdata, asiterdata_t *asidata, const char *uri);
static bool aspodcastGetPlaylist (asdata_t *asdata, asiterdata_t *asidata, const char *uri);
static bool aspodcastSongTags (asdata_t *asdata, asiterdata_t *asidata, const char *uri, const char *nm);
static void aspodcastRSS (asdata_t *asdata, asiterdata_t *asidata, const char *uri);
static int aspodcastGetClientKeyByURI (asdata_t *asdata, const char *nm);
static const char * aspodcastStripPrefix (asdata_t *asdata, const char *songuri, int clientidx);
static void audiosrcClientFree (asdata_t *asdata);

void
asiDesc (const char **ret, int max)
{
  int         c = 0;

  if (max < 2) {
    return;
  }

  /* CONTEXT: audio source: podcast */
  ret [c++] = _("Podcast");
  ret [c++] = NULL;
}

asdata_t *
asiInit (const char *delpfx, const char *origext)
{
  asdata_t    *asdata;

  asdata = mdmalloc (sizeof (asdata_t));

  asdata->webclient = NULL;
  asdata->client = ilistAlloc ("client", LIST_ORDERED);
  asdata->clientcount = 0;
  asdata->state = BDJ4_STATE_OFF;
  asdata->rssdata = NULL;
  asdata->rsslastbldtm = 0;
  return asdata;
}

void
asiFree (asdata_t *asdata)
{
  if (asdata == NULL) {
    return;
  }

  audiosrcClientFree (asdata);
  nlistFree (asdata->rssdata);
  mdfree (asdata);
}

int
asiTypeIdent (void)
{
  return AUDIOSRC_TYPE_PODCAST;
}

bool
asiIsTypeMatch (asdata_t *asdata, const char *nm)
{
  bool    rc = false;

  /* not perfect, there can be other https sources. */
  /* as the URI is stored elsewhere, the feed cannot be fetched */
  if (strncmp (nm, AS_HTTPS_PFX, AS_HTTPS_PFX_LEN) == 0) {
    rc = true;
  }

  return rc;
}

bool
asiCheckConnection (asdata_t *asdata, int askey, const char *uri)
{
  int     webrc;
  int     clientkey = -1;

  if (uri == NULL) {
    return false;
  }

  clientkey = aspodcastGetClientKeyByURI (asdata, uri);
  if (clientkey < 0) {
    return false;
  }

  webrc = webclientHead (asdata->webclient [clientkey], uri);
  if (webrc != WEB_OK) {
    return false;
  }

  return true;
}

bool
asiExists (asdata_t *asdata, const char *nm)
{
  bool    exists = false;
  int     webrc;
  char    query [1024];
  int     clientkey;

  asdata->state = BDJ4_STATE_WAIT;
  clientkey = aspodcastGetClientKeyByURI (asdata, nm);
  if (clientkey < 0) {
    return false;
  }

  stpecpy (query, query + sizeof (query), nm);

  webrc = webclientHead (asdata->webclient [clientkey], query);
  if (webrc != WEB_OK) {
    return exists;
  }

  if (asdata->state == BDJ4_STATE_PROCESS) {
    if (webrc == WEB_OK) {
      exists = true;
    }
    asdata->state = BDJ4_STATE_OFF;
  }

  return exists;
}

bool
asiPrep (asdata_t *asdata, const char *sfname, char *tempnm, size_t sz)
{
  stpecpy (tempnm, tempnm + sz, sfname);
  return true;
}

const char *
asiPrefix (asdata_t *asdata)
{
  return AS_HTTPS_PFX;
}

void
asiURI (asdata_t *asdata, const char *sfname, char *buff, size_t sz,
    const char *prefix, int pfxlen)
{
  *buff = '\0';

  if (sfname == NULL || buff == NULL) {
    return;
  }

  asiFullPath (asdata, sfname, buff, sz, NULL, 0);
}

void
asiFullPath (asdata_t *asdata, const char *sfname, char *buff, size_t sz,
    const char *prefix, int pfxlen)
{
  *buff = '\0';
  stpecpy (buff, buff + sz, sfname);
}

const char *
asiRelativePath (asdata_t *asdata, const char *sfname, int pfxlen)
{
  return sfname;
}

asiterdata_t *
asiStartIterator (asdata_t *asdata, asitertype_t asitertype,
    const char *uri, const char *nm, int askey)
{
  asiterdata_t  *asidata;

  asidata = mdmalloc (sizeof (asiterdata_t));
  asidata->songlist = NULL;
  asidata->songtags = NULL;
  asidata->iterlist = NULL;
  asidata->plNames = NULL;

  if (asitertype == AS_ITER_PL_NAMES) {
    /* a podcast only has a single playlist, use the title of the podcast */
    aspodcastGetPlaylistNames (asdata, asidata, uri);
    asidata->iterlist = asidata->plNames;
    slistStartIterator (asidata->iterlist, &asidata->iteridx);
  } else if (asitertype == AS_ITER_PL) {
    aspodcastGetPlaylist (asdata, asidata, uri);
    asidata->iterlist = asidata->songlist;
    slistStartIterator (asidata->iterlist, &asidata->iteridx);
  } else if (asitertype == AS_ITER_TAGS) {
    aspodcastSongTags (asdata, asidata, uri, nm);
    asidata->iterlist = asidata->songtags;
    slistStartIterator (asidata->iterlist, &asidata->iteridx);
  }

  return asidata;
}

void
asiCleanIterator (asdata_t *asdata, asiterdata_t *asidata)
{
  if (asidata == NULL) {
    return;
  }

  slistFree (asidata->songlist);
  slistFree (asidata->songtags);
  slistFree (asidata->plNames);
  mdfree (asidata);
}

int32_t
asiIterCount (asdata_t *asdata, asiterdata_t *asidata)
{
  int32_t   c = 0;

  if (asidata == NULL) {
    return c;
  }

  c = slistGetCount (asidata->iterlist);
  return c;
}

const char *
asiIterate (asdata_t *asdata, asiterdata_t *asidata)
{
  const char    *key = NULL;

  if (asidata == NULL) {
    return key;
  }

  key = slistIterateKey (asidata->iterlist, &asidata->iteridx);
  return key;
}

const char *
asiIterateValue (asdata_t *asdata, asiterdata_t *asidata, const char *key)
{
  const char  *val;

  if (asidata == NULL) {
    return NULL;
  }
  if (key == NULL) {
    return NULL;
  }

  val = slistGetStr (asidata->iterlist, key);
  return val;
}

/* internal routines */

static void
aspodcastWebResponseCallback (void *userdata, const char *respstr, size_t len, time_t tm)
{
  asdata_t    *asdata = (asdata_t *) userdata;

  if (asdata->state != BDJ4_STATE_WAIT) {
    asdata->state = BDJ4_STATE_OFF;
    return;
  }

  asdata->webresponse = respstr;
  asdata->webresplen = len;
  asdata->state = BDJ4_STATE_PROCESS;
  return;
}


/* internal routines */

static bool
aspodcastGetPlaylistNames (asdata_t *asdata, asiterdata_t *asidata, const char *uri)
{
  aspodcastRSS (asdata, asidata, uri);
  if (asdata->rssdata == NULL) {
    return false;
  }

  slistFree (asidata->plNames);
  asidata->plNames = slistAlloc ("asplnames", LIST_ORDERED, NULL);
  slistSetSize (asidata->plNames, 1);
  slistSetNum (asidata->plNames, nlistGetStr (asdata->rssdata, RSS_TITLE), 1);

  return true;
}

static bool
aspodcastGetPlaylist (asdata_t *asdata, asiterdata_t *asidata, const char *uri)
{
  ilist_t         *rssitems;
  ilistidx_t      iteridx;
  ilistidx_t      key;

  aspodcastRSS (asdata, asidata, uri);
  if (asdata->rssdata == NULL) {
    return false;
  }

  slistFree (asidata->songlist);
  asidata->songlist = slistAlloc ("asplsongs", LIST_UNORDERED, NULL);
  slistSetSize (asidata->songlist, nlistGetNum (asdata->rssdata, RSS_COUNT));

  rssitems = nlistGetList (asdata->rssdata, RSS_ITEMS);
  ilistStartIterator (rssitems, &iteridx);
  while ((key = ilistIterateKey (rssitems, &iteridx)) >= 0) {
    slistSetNum (asidata->songlist, ilistGetStr (rssitems, key, RSS_ITEM_URI), 1);
  }
  slistSort (asidata->songlist);

  return true;
}

static bool
aspodcastSongTags (asdata_t *asdata, asiterdata_t *asidata,
    const char *uri, const char *nm)
{
  ilist_t     *rssitems;
  slist_t     *itemidx;
  ilistidx_t  idx = -1;
  const char  *val;
  char        tbuff [40];


  aspodcastRSS (asdata, asidata, uri);
  if (asdata->rssdata == NULL) {
    return false;
  }

  rssitems = nlistGetList (asdata->rssdata, RSS_ITEMS);
  itemidx = nlistGetList (asdata->rssdata, RSS_IDX);
  idx = slistGetNum (itemidx, nm);

  if (idx < 0) {
    return false;
  }

  slistFree (asidata->songtags);
  asidata->songtags = slistAlloc ("assongtags", LIST_UNORDERED, NULL);
  slistSetSize (asidata->songtags, 4);

  val = ilistGetStr (rssitems, idx, RSS_ITEM_DURATION);
  if (val != NULL) {
    snprintf (tbuff, sizeof (tbuff), "%s000", val);
    slistSetStr (asidata->songtags, tagdefs [TAG_DURATION].tag, tbuff);
  }
  slistSetStr (asidata->songtags,
      tagdefs [TAG_TITLE].tag,
      ilistGetStr (rssitems, idx, RSS_ITEM_TITLE));
  slistSetStr (asidata->songtags,
      tagdefs [TAG_DBADDDATE].tag,
      ilistGetStr (rssitems, idx, RSS_ITEM_DATE));
  slistSetStr (asidata->songtags,
      tagdefs [TAG_NO_MAX_PLAY_TM].tag, "yes");
  slistSort (asidata->songtags);

  return true;
}

static void
aspodcastRSS (asdata_t *asdata, asiterdata_t *asidata, const char *uri)
{
  time_t    tm = 0;

  if (asdata->rssdata != NULL) {
    tm = rssGetUpdateTime (uri);
  }
  if (asdata->rssdata == NULL || tm > asdata->rsslastbldtm) {
    asdata->rssdata = rssImport (uri);
    asdata->rsslastbldtm = nlistGetNum (asdata->rssdata, RSS_BUILD_DATE);
  }
}

static int
aspodcastGetClientKeyByURI (asdata_t *asdata, const char *nm)
{
  int     clientkey = -1;
  char    temp [MAXPATHLEN];

  for (int i = 0; i < asdata->clientcount; ++i) {
    const char  *tstr;
    size_t      tlen;

    tstr = ilistGetStr (asdata->client, i, AS_CLIENT_URI);
    tlen = ilistGetNum (asdata->client, i, AS_CLIENT_URI_LEN);
    if (strncmp (nm, tstr, tlen) == 0) {
      clientkey = i;
      break;
    }
  }

  if (clientkey < 0) {
    char    *p;

    /* create a new client connection */
    clientkey = asdata->clientcount;
    asdata->clientcount += 1;
    asdata->webclient = mdmalloc (sizeof (webclient_t *) * asdata->clientcount);
    asdata->webclient [clientkey] = webclientAlloc (asdata, aspodcastWebResponseCallback);
    webclientSetTimeout (asdata->webclient [clientkey], 2);
    stpecpy (temp, temp + sizeof (temp), nm);
    p = strstr (temp + AS_HTTPS_PFX_LEN, "/");
    if (p != NULL) {
      *p = '\0';
    }
    ilistSetStr (asdata->client, clientkey, AS_CLIENT_URI, temp);
    ilistSetNum (asdata->client, clientkey, AS_CLIENT_URI_LEN, strlen (temp));
//    webclientSetUserPass (asdata->webclient [count],
//        asconfGetStr (asdata->asconf, askey, ASCONF_USER),
//        asconfGetStr (asdata->asconf, askey, ASCONF_PASS));
  }

  return clientkey;
}

static const char *
aspodcastStripPrefix (asdata_t *asdata, const char *songuri, int clientkey)
{
  const char  *turi;
  size_t      tlen;

  turi = ilistGetStr (asdata->client, clientkey, AS_CLIENT_URI);
  tlen = ilistGetNum (asdata->client, clientkey, AS_CLIENT_URI_LEN);
  if (strncmp (songuri, turi, tlen) == 0) {
    songuri += tlen;
  }
  return songuri;
}

static void
audiosrcClientFree (asdata_t *asdata)
{
  if (asdata->webclient != NULL) {
    for (int i = 0; i < asdata->clientcount; ++i) {
      webclientClose (asdata->webclient [i]);
      asdata->webclient [i] = NULL;
    }
    mdfree (asdata->webclient);
  }
  asdata->webclient = NULL;
  ilistFree (asdata->client);
  asdata->client = NULL;
  asdata->clientcount = 0;
}
