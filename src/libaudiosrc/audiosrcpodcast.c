/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
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
#include "tmutil.h"
#include "webclient.h"

enum {
  ASPODCAST_WAIT_MAX = 200,
  ASPODCAST_CLIENT_MAX = 5,
};

enum {
  AS_CLIENT_URI,
  AS_CLIENT_URI_LEN,
  AS_CLIENT_TYPE,
  AS_CLIENT_MAX,
};

typedef struct asiterdata {
  slistidx_t    iteridx;
  slist_t       *iterlist;
  slist_t       *songlist;
  slist_t       *songtags;
  slist_t       *plNames;
  char          *rssdata;
  nlist_t       *rssparse;
} asiterdata_t;

typedef struct asdata {
  webclient_t   **webclient;
  ilist_t       *client;
  const char    *webresponse;
  size_t        webresplen;
  int           clientcount;
  int           state;
} asdata_t;

static void aspodcastWebResponseCallback (void *userdata, const char *respstr, size_t len);
static int aspodcastGetClientKeyByURI (asdata_t *asdata, const char *nm);
static int aspodcastGetClientKey (asdata_t *asdata, int askey);
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
  asdata->client = NULL;
  asdata->clientcount = 0;
  asdata->state = BDJ4_STATE_OFF;
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

  /* not perfect, there can be other https/xml combinations */
  /* as the URI is stored elsewhere, the feed cannot be fetched */
  if (strncmp (nm, AS_HTTPS_PFX, AS_HTTPS_PFX_LEN) == 0) {
    size_t  len;

    len = strlen (nm);
    if (strncmp (nm + len - AS_XML_SFX_LEN, AS_XML_SFX, AS_XML_SFX_LEN) == 0) {
      rc = true;
    }
  }

  return rc;
}

bool
asiCheckConnection (asdata_t *asdata, int askey, const char *uri)
{
  int     webrc;
  char    query [1024];
  int     clientkey = 0;

  snprintf (query, sizeof (query), "%s", uri);
fprintf (stderr, "query: %s\n", query);

  webrc = webclientHead (asdata->webclient [clientkey], query);
fprintf (stderr, "webrc: %d\n", webrc);
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
  return false;
}

void
asiPrepClean (asdata_t *asdata, const char *tempnm)
{
  return;
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
asiStartIterator (asdata_t *asdata, asitertype_t asitertype, const char *nm, int askey)
{
  asiterdata_t  *asidata;

  asidata = mdmalloc (sizeof (asiterdata_t));
  asidata->songlist = NULL;
  asidata->songtags = NULL;
  asidata->iterlist = NULL;
  asidata->plNames = NULL;

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
aspodcastWebResponseCallback (void *userdata, const char *respstr, size_t len)
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

static int
aspodcastGetClientKeyByURI (asdata_t *asdata, const char *nm)
{
  int     clientkey = -1;

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
