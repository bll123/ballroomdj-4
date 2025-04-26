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

#include "asconf.h"
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
#include "pathinfo.h"
#include "playlist.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"
#include "webclient.h"

enum {
  ASPODCAST_WAIT_MAX = 200,
  ASPODCAST_CLIENT_MAX = 3,
};

enum {
  AS_CLIENT_URI,
  AS_CLIENT_URI_LEN,
  AS_CLIENT_ASKEY,
  AS_CLIENT_TYPE,
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
  asconf_t      *asconf;
  webclient_t   **webclient;
  ilist_t       *client;
  const char    *webresponse;
  size_t        webresplen;
  int           clientcount;
  int           state;
} asdata_t;

static void aspodcastWebResponseCallback (void *userdata, const char *respstr, size_t len);
static bool aspodcastGetPlaylist (asdata_t *asdata, asiterdata_t *asidata, const char *nm, int askey);
static bool aspodcastSongTags (asdata_t *asdata, asiterdata_t *asidata, const char *songuri);
static bool aspodcastGetPlaylistNames (asdata_t *asdata, asiterdata_t *asidata, int askey);
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

  ret [c++] = "Podcast";
  ret [c++] = NULL;
}

asdata_t *
asiInit (const char *delpfx, const char *origext)
{
  asdata_t    *asdata;

  asdata = mdmalloc (sizeof (asdata_t));

  asdata->asconf = asconfAlloc ();
  asdata->webclient = NULL;
  asdata->client = NULL;
  asdata->clientcount = 0;
  asdata->state = BDJ4_STATE_OFF;
  return asdata;
}

void
asiPostInit (asdata_t *asdata, const char *uri)
{
  ilistidx_t    iteridx;
  ilistidx_t    askey;
  int           count = 0;
  char          temp [MAXPATHLEN];

  asconfStartIterator (asdata->asconf, &iteridx);
  while ((askey = asconfIterate (asdata->asconf, &iteridx)) >= 0) {
    if (asconfGetNum (asdata->asconf, askey, ASCONF_MODE) == ASCONF_MODE_CLIENT) {
      int   type;

      type = asconfGetNum (asdata->asconf, askey, ASCONF_TYPE);
      if (type == AUDIOSRC_TYPE_PODCAST) {
        ++count;
      }
    }
  }

  audiosrcClientFree (asdata);

  asdata->clientcount = count;
  asdata->webclient = mdmalloc (sizeof (webclient_t *) * count);
  asdata->client = ilistAlloc ("client", LIST_ORDERED);

  count = 0;
  asconfStartIterator (asdata->asconf, &iteridx);
  while ((askey = asconfIterate (asdata->asconf, &iteridx)) >= 0) {
    if (asconfGetNum (asdata->asconf, askey, ASCONF_MODE) == ASCONF_MODE_CLIENT) {
      int   type;

      type = asconfGetNum (asdata->asconf, askey, ASCONF_TYPE);
      if (type == AUDIOSRC_TYPE_PODCAST) {
        const char    *user;
        const char    *pass;

        asdata->webclient [count] = webclientAlloc (asdata, aspodcastWebResponseCallback);
        webclientIgnoreCertErr (asdata->webclient [count]);
        webclientSetTimeout (asdata->webclient [count], 1);
        snprintf (temp, sizeof (temp),
            "%s%s/",
            AS_HTTPS_PFX,
            asconfGetStr (asdata->asconf, askey, ASCONF_URI));
        ilistSetStr (asdata->client, count, AS_CLIENT_URI, temp);
        ilistSetNum (asdata->client, count, AS_CLIENT_URI_LEN, strlen (temp));
        ilistSetNum (asdata->client, count, AS_CLIENT_ASKEY, askey);
        ilistSetNum (asdata->client, count, AS_CLIENT_TYPE, type);
        user = asconfGetStr (asdata->asconf, askey, ASCONF_USER);
        pass = asconfGetStr (asdata->asconf, askey, ASCONF_PASS);
        if (user != NULL && *user && pass != NULL && *pass) {
          webclientSetUserPass (asdata->webclient [count], user, pass);
        }
        ++count;
      }
    }
  }

  return;
}

void
asiFree (asdata_t *asdata)
{
  if (asdata == NULL) {
    return;
  }

  audiosrcClientFree (asdata);
  asconfFree (asdata->asconf);
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

  /* this doesn't really work properly, as there are */
  /* multiple types that use https. */
  /* will need to depend on the song being marked as a podcast */
  if (strncmp (nm, AS_HTTPS_PFX, AS_HTTPS_PFX_LEN) == 0) {
    rc = true;
  }
  if (strncmp (nm, AS_HTTP_PFX, AS_HTTP_PFX_LEN) == 0) {
    rc = true;
  }

  return rc;
}

bool
asiCheckConnection (asdata_t *asdata, int askey)
{
  int     webrc;
  char    query [1024];
  int     clientkey;

  asconfFree (asdata->asconf);
  asdata->asconf = asconfAlloc ();
  asiPostInit (asdata, NULL);

  snprintf (query, sizeof (query), "%s",
      ilistGetStr (asdata->client, askey, AS_CLIENT_URI));

  webrc = webclientHead (asdata->webclient [clientkey], query);
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

  if (asitertype == AS_ITER_PL_NAMES) {
    aspodcastGetPlaylistNames (asdata, asidata, askey);
    asidata->iterlist = asidata->plNames;
    slistStartIterator (asidata->iterlist, &asidata->iteridx);
  } else if (asitertype == AS_ITER_PL) {
    aspodcastGetPlaylist (asdata, asidata, nm, askey);
    asidata->iterlist = asidata->songlist;
    slistStartIterator (asidata->iterlist, &asidata->iteridx);
  } else if (asitertype == AS_ITER_TAGS) {
    aspodcastSongTags (asdata, asidata, nm);
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


static bool
aspodcastGetPlaylist (asdata_t *asdata, asiterdata_t *asidata, const char *nm, int askey)
{
  bool    rc = false;
  int     webrc;
  char    query [1024];
  int     clientkey = -1;

  clientkey = aspodcastGetClientKey (asdata, askey);
  if (clientkey < 0) {
    return false;
  }

#if 0
  asdata->state = BDJ4_STATE_WAIT;


  webrc = webclientGet (asdata->webclient [clientkey], query);
  if (webrc != WEB_OK) {
    return rc;
  }

  if (asdata->state == BDJ4_STATE_PROCESS) {
    if (asdata->webresplen > 0) {
      char    *tdata;
      char    *p;
      char    *tokstr;

      tdata = mdmalloc (asdata->webresplen + 1);
      memcpy (tdata, asdata->webresponse, asdata->webresplen);
      tdata [asdata->webresplen] = '\0';
      slistFree (asidata->songlist);
      asidata->songlist = slistAlloc ("asplsongs", LIST_UNORDERED, NULL);


      rc = true;
    }
    asdata->state = BDJ4_STATE_OFF;
  }
#endif

  return rc;
}

static bool
aspodcastSongTags (asdata_t *asdata, asiterdata_t *asidata, const char *songuri)
{
  bool    rc = false;
  int     webrc;
  char    query [1024];
  int     clientkey;

  asdata->state = BDJ4_STATE_WAIT;
  clientkey = aspodcastGetClientKeyByURI (asdata, songuri);
  if (clientkey < 0) {
    return false;
  }

#if 0
  snprintf (query, sizeof (query),
      "%s%s"
      "?uri=%s",
      ilistGetStr (asdata->client, clientkey, AS_CLIENT_URI),
      action_str [asdata->action],
      aspodcastStripPrefix (asdata, songuri, clientkey));

  webrc = webclientGet (asdata->webclient [clientkey], query);
  if (webrc != WEB_OK) {
    return rc;
  }

  if (asdata->state == BDJ4_STATE_PROCESS) {
    if (asdata->webresplen > 0) {
      char    *tdata;
      char    *p;
      char    *tokstr;

      tdata = mdmalloc (asdata->webresplen + 1);
      memcpy (tdata, asdata->webresponse, asdata->webresplen);
      tdata [asdata->webresplen] = '\0';
      slistFree (asidata->songtags);
      asidata->songtags = slistAlloc ("asplsongs", LIST_UNORDERED, NULL);

      p = strtok_r (tdata, MSG_ARGS_RS_STR, &tokstr);
      while (p != NULL) {
        const char  *tval;

        tval = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
        if (tval != NULL) {
          if (*tval == MSG_ARGS_EMPTY) {
            tval = NULL;
          }
          slistSetStr (asidata->songtags, p, tval);
        }
        p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
      }

      rc = true;
    }
    asdata->state = BDJ4_STATE_OFF;
  }
#endif

  return rc;
}

static bool
aspodcastGetPlaylistNames (asdata_t *asdata, asiterdata_t *asidata, int askey)
{
  bool    rc = false;
  int     webrc;
  char    query [1024];
  int     clientkey = -1;

  clientkey = aspodcastGetClientKey (asdata, askey);
  if (clientkey < 0) {
    return false;
  }

#if 0
  asdata->state = BDJ4_STATE_WAIT;
  snprintf (query, sizeof (query),
      "%s%s",
      ilistGetStr (asdata->client, clientkey, AS_CLIENT_BDJ4_URI),
      action_str [asdata->action]);

  webrc = webclientGet (asdata->webclient [clientkey], query);
  if (webrc != WEB_OK) {
    return rc;
  }

  if (asdata->state == BDJ4_STATE_PROCESS) {
    if (asdata->webresplen > 0) {
      char    *tdata;
      char    *p;
      char    *tokstr = NULL;

      tdata = mdmalloc (asdata->webresplen + 1);
      memcpy (tdata, asdata->webresponse, asdata->webresplen);
      tdata [asdata->webresplen] = '\0';
      slistFree (asidata->plNames);
      asidata->plNames = slistAlloc ("asplnames", LIST_UNORDERED, NULL);

      p = strtok_r (tdata, MSG_ARGS_RS_STR, &tokstr);
      while (p != NULL) {
        slistSetNum (asidata->plNames, p, 1);
        p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
      }
      slistSort (asidata->plNames);
      rc = true;
      mdfree (tdata);
    }
    asdata->state = BDJ4_STATE_OFF;
  }
#endif

  return rc;
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

int
aspodcastGetClientKey (asdata_t *asdata, int askey)
{
  int   clientkey = -1;

  for (int i = 0; i < asdata->clientcount; ++i) {
    int   tkey;

    tkey = ilistGetNum (asdata->client, i, AS_CLIENT_ASKEY);
    if (tkey == askey) {
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
