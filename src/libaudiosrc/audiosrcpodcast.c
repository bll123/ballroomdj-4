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
  ASPODCAST_RECHK_TM = 120,
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
  slist_t       *plData;
} asiterdata_t;

typedef struct {
  webclient_t   *webclient;
  nlist_t       *rssdata;
  time_t        rsslastbldtm;
  time_t        rsslastchktm;
} asclientdata_t;

typedef struct asdata {
  asclientdata_t  *clientdata;
  ilist_t         *client;
  const char      *webresponse;
  size_t          webresplen;
  int             clientcount;
  int             state;
} asdata_t;

static void aspodcastWebResponseCallback (void *userdata, const char *respstr, size_t len, time_t tm);
static bool aspodcastGetPlaylistNames (asdata_t *asdata, asiterdata_t *asidata, const char *uri);
static bool aspodcastGetPlaylist (asdata_t *asdata, asiterdata_t *asidata, const char *uri);
static bool aspodcastSongTags (asdata_t *asdata, asiterdata_t *asidata, const char *uri, const char *nm);
static void aspodcastRSS (asdata_t *asdata, asiterdata_t *asidata, const char *uri);
static int aspodcastGetClientKeyByURI (asdata_t *asdata, const char *uri);
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

  asdata->clientdata = NULL;
  asdata->client = ilistAlloc ("client", LIST_ORDERED);
  asdata->clientcount = 0;
  asdata->state = BDJ4_STATE_OFF;
  asdata->webresponse = NULL;
  asdata->webresplen = 0;
  return asdata;
}

void
asiFree (asdata_t *asdata)
{
  if (asdata == NULL) {
    return;
  }

  audiosrcClientFree (asdata);
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

  if (uri == NULL || ! *uri) {
    return false;
  }

  clientkey = aspodcastGetClientKeyByURI (asdata, uri);
  if (clientkey < 0) {
    return false;
  }

  webrc = webclientHead (asdata->clientdata [clientkey].webclient, uri);
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

  webrc = webclientHead (asdata->clientdata [clientkey].webclient, query);
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

/* asiPrep is called in a multi-threaded context */
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
  asidata->plData = NULL;

  if (asitertype == AS_ITER_PL_NAMES) {
    /* a podcast only has a single playlist, use the title of the podcast */
    aspodcastGetPlaylistNames (asdata, asidata, uri);
    asidata->iterlist = asidata->plNames;
    slistStartIterator (asidata->iterlist, &asidata->iteridx);
  } else if (asitertype == AS_ITER_PL_DATA) {
    aspodcastGetPlaylist (asdata, asidata, uri);
    asidata->iterlist = asidata->plData;
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
  slistFree (asidata->plData);
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
aspodcastGetPlaylistNames (asdata_t *asdata, asiterdata_t *asidata,
    const char *uri)
{
  int     clientkey;

  clientkey = aspodcastGetClientKeyByURI (asdata, uri);
  if (clientkey < 0) {
    return false;
  }

  aspodcastRSS (asdata, asidata, uri);

  if (asdata->clientdata [clientkey].rssdata == NULL) {
    return false;
  }

  slistFree (asidata->plNames);
  asidata->plNames = slistAlloc ("asplnames", LIST_ORDERED, NULL);
  slistSetSize (asidata->plNames, 1);
  slistSetNum (asidata->plNames,
      nlistGetStr (asdata->clientdata [clientkey].rssdata, RSS_TITLE), 1);

  return true;
}

static bool
aspodcastGetPlaylist (asdata_t *asdata, asiterdata_t *asidata, const char *uri)
{
  ilist_t         *rssitems;
  ilistidx_t      iteridx;
  ilistidx_t      key;
  int             clientkey;
  nlist_t         *tlist;
  nlistidx_t      titeridx;
  const char      *tstr;

  clientkey = aspodcastGetClientKeyByURI (asdata, uri);
  if (clientkey < 0) {
    return false;
  }

  aspodcastRSS (asdata, asidata, uri);
  if (asdata->clientdata [clientkey].rssdata == NULL) {
    return false;
  }

  slistFree (asidata->plData);
  asidata->plData = slistAlloc ("aspldata", LIST_UNORDERED, NULL);
  slistSetStr (asidata->plData, "IMAGE_URI",
      nlistGetStr (asdata->clientdata [clientkey].rssdata, RSS_IMAGE_URI));
  slistSort (asidata->plData);

  slistFree (asidata->songlist);
  asidata->songlist = slistAlloc ("asplsongs", LIST_UNORDERED, NULL);
  slistSetSize (asidata->songlist,
      nlistGetNum (asdata->clientdata [clientkey].rssdata, RSS_COUNT));
  tlist = nlistAlloc ("tmp-asplsongs", LIST_UNORDERED, NULL);
  nlistSetSize (tlist,
      nlistGetNum (asdata->clientdata [clientkey].rssdata, RSS_COUNT));

  rssitems = nlistGetList (asdata->clientdata [clientkey].rssdata, RSS_ITEMS);
  ilistStartIterator (rssitems, &iteridx);
  while ((key = ilistIterateKey (rssitems, &iteridx)) >= 0) {
    int64_t   val;

    val = atoll (ilistGetStr (rssitems, key, RSS_ITEM_DATE));
    /* reverse sort */
    val = ~val;
    nlistSetStr (tlist, val, ilistGetStr (rssitems, key, RSS_ITEM_URI));
  }

  nlistSort (tlist);
  nlistStartIterator (tlist, &titeridx);
  while ((tstr = nlistIterateValueData (tlist, &titeridx)) != NULL) {
    slistSetNum (asidata->songlist, tstr, 1);
  }
  nlistFree (tlist);

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
  int         clientkey;

  clientkey = aspodcastGetClientKeyByURI (asdata, uri);
  if (clientkey < 0) {
    return false;
  }

  aspodcastRSS (asdata, asidata, uri);
  if (asdata->clientdata [clientkey].rssdata == NULL) {
    return false;
  }

  rssitems = nlistGetList (asdata->clientdata [clientkey].rssdata, RSS_ITEMS);
  itemidx = nlistGetList (asdata->clientdata [clientkey].rssdata, RSS_IDX);
  idx = slistGetNum (itemidx, nm);

  if (idx < 0) {
    return false;
  }

  slistFree (asidata->songtags);
  asidata->songtags = slistAlloc ("assongtags", LIST_UNORDERED, NULL);
  slistSetSize (asidata->songtags, 5);

  *tbuff = '\0';
  val = ilistGetStr (rssitems, idx, RSS_ITEM_DURATION);
  if (val != NULL) {
    if (strchr (val, ':') != NULL) {
      long  a, b, c;
      int   rc;

      rc = sscanf (val, "%ld:%ld:%ld", &a, &b, &c);
      if (rc == 3) {
        snprintf (tbuff, sizeof (tbuff), "%ld000", a * 60 * 60 + b * 60 + c);
      } else if (rc == 2) {
        snprintf (tbuff, sizeof (tbuff), "%ld000", a * 60 + b);
      }
    } else {
      snprintf (tbuff, sizeof (tbuff), "%s000", val);
    }
    slistSetStr (asidata->songtags, tagdefs [TAG_DURATION].tag, tbuff);
  }
  slistSetStr (asidata->songtags,
      tagdefs [TAG_TITLE].tag,
      ilistGetStr (rssitems, idx, RSS_ITEM_TITLE));
  slistSetStr (asidata->songtags,
      tagdefs [TAG_DBADDDATE].tag,
      ilistGetStr (rssitems, idx, RSS_ITEM_DATE));
  slistSetStr (asidata->songtags,
      tagdefs [TAG_NO_PLAY_TM_LIMIT].tag, "yes");
  slistSetStr (asidata->songtags,
      tagdefs [TAG_SONG_TYPE].tag, "podcast");

  slistSort (asidata->songtags);

  return true;
}

static void
aspodcastRSS (asdata_t *asdata, asiterdata_t *asidata, const char *uri)
{
  time_t    tm = 0;
  int       clientkey;

  clientkey = aspodcastGetClientKeyByURI (asdata, uri);
  if (clientkey < 0) {
    return;
  }

  if (asdata->clientdata [clientkey].rssdata != NULL) {
    time_t    currtm;
    time_t    lastchk;

    currtm = time (NULL);
    lastchk = asdata->clientdata [clientkey].rsslastchktm;
    tm = asdata->clientdata [clientkey].rsslastbldtm;
    if (currtm > lastchk + ASPODCAST_RECHK_TM) {
      tm = rssGetUpdateTime (uri);
    }
  }
  if (asdata->clientdata [clientkey].rssdata == NULL ||
      tm > asdata->clientdata [clientkey].rsslastbldtm) {
    logMsg (LOG_ERR, LOG_IMPORTANT,
        "do import: rss data null(%d) or time %" PRId64 " > %" PRId64,
        asdata->clientdata [clientkey].rssdata == NULL,
        (int64_t) tm, (int64_t) asdata->clientdata [clientkey].rsslastbldtm);
    asdata->clientdata [clientkey].rssdata = rssImport (uri);
    asdata->clientdata [clientkey].rsslastbldtm =
        nlistGetNum (asdata->clientdata [clientkey].rssdata, RSS_BUILD_DATE);
    asdata->clientdata [clientkey].rsslastchktm = time (NULL);
  }
}

static int
aspodcastGetClientKeyByURI (asdata_t *asdata, const char *uri)
{
  int     clientkey = -1;

  if (uri == NULL || ! *uri) {
    return clientkey;
  }

  for (int i = 0; i < asdata->clientcount; ++i) {
    const char  *tstr;
    size_t      tlen;

    tstr = ilistGetStr (asdata->client, i, AS_CLIENT_URI);
    tlen = ilistGetNum (asdata->client, i, AS_CLIENT_URI_LEN);
    if (strncmp (uri, tstr, tlen) == 0) {
      clientkey = i;
      break;
    }
  }

  if (clientkey < 0) {
    char    temp [MAXPATHLEN];
    char    *p;

    /* create a new client connection */
    clientkey = asdata->clientcount;
    asdata->clientcount += 1;
    asdata->clientdata = mdrealloc (asdata->clientdata,
        sizeof (asclientdata_t) * asdata->clientcount);
    asdata->clientdata [clientkey].webclient =
        webclientAlloc (asdata, aspodcastWebResponseCallback);
    asdata->clientdata [clientkey].rssdata = NULL;
    asdata->clientdata [clientkey].rsslastbldtm = 0;
    stpecpy (temp, temp + sizeof (temp), uri);
    p = temp;
    p += AS_HTTPS_PFX_LEN;
    p = strstr (p, "/");
    if (p != NULL) {
      *p = '\0';
    }
    ilistSetStr (asdata->client, clientkey, AS_CLIENT_URI, temp);
    ilistSetNum (asdata->client, clientkey, AS_CLIENT_URI_LEN, strlen (temp));
//    webclientSetUserPass (asdata->clientdata [count].webclient,
//        asconfGetStr (asdata->asconf, askey, ASCONF_USER),
//        asconfGetStr (asdata->asconf, askey, ASCONF_PASS));
  }

  return clientkey;
}

static void
audiosrcClientFree (asdata_t *asdata)
{
  if (asdata->clientdata != NULL) {
    for (int i = 0; i < asdata->clientcount; ++i) {
      webclientClose (asdata->clientdata [i].webclient);
      nlistFree (asdata->clientdata [i].rssdata);
    }
    mdfree (asdata->clientdata);
  }
  asdata->clientdata = NULL;
  ilistFree (asdata->client);
  asdata->client = NULL;
  asdata->clientcount = 0;
}
