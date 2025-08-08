/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include "asconf.h"
#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
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
  ASBDJ4_ACT_NONE,
  ASBDJ4_ACT_ECHO,
  ASBDJ4_ACT_GET_PLAYLIST,
  ASBDJ4_ACT_GET_PL_NAMES,
  ASBDJ4_ACT_GET_SONG,
  ASBDJ4_ACT_SONG_EXISTS,
  ASBDJ4_ACT_SONG_TAGS,
  ASBDJ4_ACT_MAX,
};

enum {
  ASBDJ4_WAIT_MAX = 200,
};

const char *action_str [ASBDJ4_ACT_MAX] = {
  [ASBDJ4_ACT_NONE] = "none",
  [ASBDJ4_ACT_ECHO] = "echo",
  [ASBDJ4_ACT_GET_PLAYLIST] = "plget",
  [ASBDJ4_ACT_GET_PL_NAMES] = "plnames",
  [ASBDJ4_ACT_GET_SONG] = "songget",
  [ASBDJ4_ACT_SONG_EXISTS] = "songexists",
  [ASBDJ4_ACT_SONG_TAGS] = "songtags",
};

enum {
  AS_CLIENT_REMOTE_URI,
  AS_CLIENT_URI,
  AS_CLIENT_URI_LEN,
  AS_CLIENT_ASKEY,
  AS_CLIENT_MAX,
};

typedef struct {
  webclient_t   *webclient;
  const char    *webresponse;
  const char    *remoteuri;
  const char    *uri;
  size_t        webresplen;
  int           idx;
  int           uriidx;
  int           action;
  int           state;
  int           askey;
  int           clientkey;
  _Atomic(bool) inuse;
} asclientdata_t;

typedef struct asiterdata {
  slistidx_t    iteridx;
  slist_t       *iterlist;
  slist_t       *songlist;
  slist_t       *songtags;
  slist_t       *plNames;
} asiterdata_t;

typedef struct asdata {
  asconf_t        *asconf;
  asclientdata_t  **clientdata;
  ilist_t         *urilist;
  int             clientcount;
  volatile atomic_flag locked;
} asdata_t;

static void asbdj4WebResponseCallback (void *userdata, const char *respstr, size_t len, time_t tm);
static bool asbdj4GetPlaylist (asdata_t *asdata, asiterdata_t *asidata, const char *nm, int askey);
static bool asbdj4SongTags (asdata_t *asdata, asiterdata_t *asidata, const char *songuri);
static bool asbdj4GetPlaylistNames (asdata_t *asdata, asiterdata_t *asidata, int askey);
static bool asbdj4GetAudioFile (asdata_t *asdata, const char *nm, const char *tempnm);
static int asbdj4GetClientKeyByURI (asdata_t *asdata, const char *nm);
static int asbdj4GetClientKey (asdata_t *asdata, int askey);
static const char * asbdj4StripPrefix (asdata_t *asdata, const char *songuri, int clientidx);
static void audiosrcClientFree (asdata_t *asdata);
static int asbdj4ClientInit (asdata_t *asdata, int askey);

void
asiDesc (const char **ret, int max)
{
  int         c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "BDJ4";
  ret [c++] = NULL;
}

asdata_t *
asiInit (const char *delpfx, const char *origext)
{
  asdata_t      *asdata;
  ilistidx_t    iteridx;
  ilistidx_t    askey;
  int           count = 0;
  char          temp [MAXPATHLEN];

  asdata = mdmalloc (sizeof (asdata_t));

  asdata->asconf = asconfAlloc ();
  asdata->clientdata = NULL;
  asdata->urilist = NULL;
  asdata->clientcount = 0;
  atomic_flag_clear (&asdata->locked);

  asconfStartIterator (asdata->asconf, &iteridx);
  while ((askey = asconfIterate (asdata->asconf, &iteridx)) >= 0) {
    int   type;

    if (asconfGetNum (asdata->asconf, askey, ASCONF_MODE) != ASCONF_MODE_CLIENT) {
      continue;
    }

    type = asconfGetNum (asdata->asconf, askey, ASCONF_TYPE);
    if (type == AUDIOSRC_TYPE_BDJ4) {
      ++count;
    }
  }

  asdata->clientcount = 0;
  asdata->urilist = ilistAlloc ("client", LIST_ORDERED);
  ilistSetSize (asdata->urilist, count);

  if (count == 0) {
    return asdata;
  }

  count = 0;
  asconfStartIterator (asdata->asconf, &iteridx);
  while ((askey = asconfIterate (asdata->asconf, &iteridx)) >= 0) {
    int   type;

    if (asconfGetNum (asdata->asconf, askey, ASCONF_MODE) != ASCONF_MODE_CLIENT) {
      continue;
    }

    type = asconfGetNum (asdata->asconf, askey, ASCONF_TYPE);
    if (type != AUDIOSRC_TYPE_BDJ4) {
      continue;
    }

    snprintf (temp, sizeof (temp),
        "https://%s:%" PRIu16 "/",
        asconfGetStr (asdata->asconf, askey, ASCONF_URI),
        (uint16_t) asconfGetNum (asdata->asconf, askey, ASCONF_PORT));
    ilistSetStr (asdata->urilist, count, AS_CLIENT_REMOTE_URI, temp);
    snprintf (temp, sizeof (temp),
        "%s%s:%" PRIu16 "/",
        AS_BDJ4_PFX,
        asconfGetStr (asdata->asconf, askey, ASCONF_URI),
        (uint16_t) asconfGetNum (asdata->asconf, askey, ASCONF_PORT));
    ilistSetStr (asdata->urilist, count, AS_CLIENT_URI, temp);
    ilistSetNum (asdata->urilist, count, AS_CLIENT_URI_LEN, strlen (temp));
    ilistSetNum (asdata->urilist, count, AS_CLIENT_ASKEY, askey);
    ++count;
  }

  return asdata;
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
  return AUDIOSRC_TYPE_BDJ4;
}

bool
asiIsTypeMatch (asdata_t *asdata, const char *nm)
{
  bool    rc = false;

  if (strncmp (nm, AS_BDJ4_PFX, AS_BDJ4_PFX_LEN) == 0) {
    rc = true;
  }

  return rc;
}

bool
asiCheckConnection (asdata_t *asdata, int askey, const char *uri)
{
  int             webrc;
  char            query [1024];
  int             clientkey;
  asclientdata_t  *clientdata;

  /* reload the audiosrc.txt datafile */
  asconfFree (asdata->asconf);
  asdata->asconf = asconfAlloc ();

  clientkey = asbdj4GetClientKey (asdata, askey);
  if (clientkey < 0) {
    return false;
  }
  clientdata = asdata->clientdata [clientkey];
  clientdata->action = ASBDJ4_ACT_ECHO;
  clientdata->state = BDJ4_STATE_WAIT;

  snprintf (query, sizeof (query),
      "%s%s",
      clientdata->remoteuri, action_str [clientdata->action]);

  clientdata->inuse = true;
  webclientSetTimeout (clientdata->webclient, 1000);
  webrc = webclientGet (clientdata->webclient, query);
  webclientSetTimeout (clientdata->webclient, 0);
  clientdata->inuse = false;
  if (webrc != WEB_OK) {
    return false;
  }

  return true;
}

bool
asiExists (asdata_t *asdata, const char *nm)
{
  bool            exists = false;
  int             webrc;
  char            uri [1024];
  char            query [MAXPATHLEN];
  int             clientkey;
  asclientdata_t  *clientdata;

  clientkey = asbdj4GetClientKeyByURI (asdata, nm);
  if (clientkey < 0) {
    return false;
  }
  clientdata = asdata->clientdata [clientkey];
  clientdata->action = ASBDJ4_ACT_SONG_EXISTS;
  clientdata->state = BDJ4_STATE_WAIT;

  snprintf (uri, sizeof (uri),
      "%s%s",
      clientdata->remoteuri, action_str [clientdata->action]);
  snprintf (query, sizeof (query),
      "uri=%s",
      asbdj4StripPrefix (asdata, nm, clientkey));

  clientdata->inuse = true;
  webrc = webclientPost (clientdata->webclient, uri, query);
  if (webrc != WEB_OK) {
    clientdata->inuse = false;
    return exists;
  }

  if (clientdata->state == BDJ4_STATE_PROCESS) {
    if (webrc == WEB_OK) {
      exists = true;
    }
    clientdata->state = BDJ4_STATE_OFF;
  }

  clientdata->inuse = false;
  return exists;
}

/* asiPrep is called in a multi-threaded context */
bool
asiPrep (asdata_t *asdata, const char *sfname, char *tempnm, size_t sz)
{
  mstime_t  mstm;
  time_t    tm;
  int       rc = false;

  if (sfname == NULL || tempnm == NULL) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "WARN: prep: null-data");
    return false;
  }

  mstimestart (&mstm);
  audiosrcutilMakeTempName (sfname, tempnm, sz);
  fileopDelete (tempnm);

  rc = asbdj4GetAudioFile (asdata, sfname, tempnm);
  /* the file should already be in the disk cache as it was just written */

  if (! fileopFileExists (tempnm)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file copy failed: %s", tempnm);
    return false;
  }

  tm = mstimeend (&mstm);
  logMsg (LOG_DBG, LOG_BASIC, "asbdj4: prep-time (%" PRIu64 ") %s", (uint64_t) tm, sfname);

  return rc;
}

void
asiPrepClean (asdata_t *asdata, const char *tempnm)
{
  if (tempnm == NULL) {
    return;
  }

  if (fileopFileExists (tempnm)) {
    fileopDelete (tempnm);
  }
}

const char *
asiPrefix (asdata_t *asdata)
{
  return AS_BDJ4_PFX;
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

  logMsg (LOG_DBG, LOG_AUDIOSRC, "start-iter: type: %d askey: %d %s %s",
      asitertype, askey, uri, nm);
  if (asitertype == AS_ITER_PL_NAMES) {
    asbdj4GetPlaylistNames (asdata, asidata, askey);
    asidata->iterlist = asidata->plNames;
    slistStartIterator (asidata->iterlist, &asidata->iteridx);
  } else if (asitertype == AS_ITER_PL) {
    asbdj4GetPlaylist (asdata, asidata, nm, askey);
    asidata->iterlist = asidata->songlist;
    slistStartIterator (asidata->iterlist, &asidata->iteridx);
  } else if (asitertype == AS_ITER_TAGS) {
    asbdj4SongTags (asdata, asidata, nm);
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
asbdj4WebResponseCallback (void *userdata, const char *respstr, size_t len, time_t tm)
{
  asclientdata_t  *clientdata = (asclientdata_t *) userdata;

  if (clientdata->action == ASBDJ4_ACT_NONE) {
    clientdata->state = BDJ4_STATE_OFF;
    return;
  }

  if (clientdata->state != BDJ4_STATE_WAIT) {
    clientdata->state = BDJ4_STATE_OFF;
    return;
  }

  clientdata->webresponse = respstr;
  clientdata->webresplen = len;
  clientdata->state = BDJ4_STATE_PROCESS;
  return;
}


static bool
asbdj4GetPlaylist (asdata_t *asdata, asiterdata_t *asidata, const char *nm, int askey)
{
  bool            rc = false;
  int             webrc;
  char            uri [1024];
  char            query [MAXPATHLEN];
  int             clientkey = -1;
  asclientdata_t  *clientdata;

  logMsg (LOG_DBG, LOG_AUDIOSRC, "pl-get: askey: %d", askey);
  clientkey = asbdj4GetClientKey (asdata, askey);
  if (clientkey < 0) {
    return false;
  }
  logMsg (LOG_DBG, LOG_AUDIOSRC, "pl-get: clientkey: %d", clientkey);

  clientdata = asdata->clientdata [clientkey];
  clientdata->action = ASBDJ4_ACT_GET_PLAYLIST;
  clientdata->state = BDJ4_STATE_WAIT;

  snprintf (uri, sizeof (uri),
      "%s%s",
      clientdata->remoteuri, action_str [clientdata->action]);
  snprintf (query, sizeof (query),
      "uri=%s",
      asbdj4StripPrefix (asdata, nm, clientkey));

  clientdata->inuse = true;
  webrc = webclientPost (clientdata->webclient, uri, query);
  if (webrc != WEB_OK) {
    clientdata->inuse = false;
    return rc;
  }

  if (clientdata->state == BDJ4_STATE_PROCESS) {
    if (clientdata->webresplen > 0) {
      char    *tdata;
      char    *p;
      char    *tokstr;

      tdata = mdmalloc (clientdata->webresplen + 1);
      memcpy (tdata, clientdata->webresponse, clientdata->webresplen);
      tdata [clientdata->webresplen] = '\0';
      slistFree (asidata->songlist);
      asidata->songlist = slistAlloc ("asplsongs", LIST_UNORDERED, NULL);

      p = strtok_r (tdata, MSG_ARGS_RS_STR, &tokstr);
      while (p != NULL) {
        char    tbuff [MAXPATHLEN];

        logMsg (LOG_DBG, LOG_AUDIOSRC, "pl-get: %s", p);
        snprintf (tbuff, sizeof (tbuff), "%s%s",
            clientdata->uri, p);
        slistSetNum (asidata->songlist, tbuff, 1);
        p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
      }

      rc = true;
    }
    clientdata->state = BDJ4_STATE_OFF;
  }

  clientdata->inuse = false;
  return rc;
}

static bool
asbdj4SongTags (asdata_t *asdata, asiterdata_t *asidata, const char *songuri)
{
  bool            rc = false;
  int             webrc;
  char            uri [1024];
  char            query [MAXPATHLEN];
  int             clientkey;
  asclientdata_t  *clientdata;

  clientkey = asbdj4GetClientKeyByURI (asdata, songuri);
  if (clientkey < 0) {
    return false;
  }
  logMsg (LOG_DBG, LOG_AUDIOSRC, "song-tags: clientkey: %d", clientkey);
  clientdata = asdata->clientdata [clientkey];
  clientdata->action = ASBDJ4_ACT_SONG_TAGS;
  clientdata->state = BDJ4_STATE_WAIT;

  /* the query field may contain weird characters, use a post */
  snprintf (uri, sizeof (uri),
      "%s%s",
      clientdata->remoteuri, action_str [clientdata->action]);
  snprintf (query, sizeof (query),
      "uri=%s",
      asbdj4StripPrefix (asdata, songuri, clientkey));

  clientdata->inuse = true;
  webrc = webclientPost (clientdata->webclient, uri, query);
  if (webrc != WEB_OK) {
    clientdata->inuse = false;
    return rc;
  }

  if (clientdata->state == BDJ4_STATE_PROCESS) {
    if (clientdata->webresplen > 0) {
      char    *tdata;
      char    *p;
      char    *tokstr;

      tdata = mdmalloc (clientdata->webresplen + 1);
      memcpy (tdata, clientdata->webresponse, clientdata->webresplen);
      tdata [clientdata->webresplen] = '\0';
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
    clientdata->state = BDJ4_STATE_OFF;
  }

  slistSetStr (asidata->songtags, "SONGTYPE", "remote");

  clientdata->inuse = false;
  return rc;
}

static bool
asbdj4GetPlaylistNames (asdata_t *asdata, asiterdata_t *asidata, int askey)
{
  bool            rc = false;
  int             webrc;
  char            uri [1024];
  int             clientkey = -1;
  asclientdata_t  *clientdata;

  clientkey = asbdj4GetClientKey (asdata, askey);
  if (clientkey < 0) {
    return false;
  }
  clientdata = asdata->clientdata [clientkey];
  clientdata->action = ASBDJ4_ACT_GET_PL_NAMES;
  clientdata->state = BDJ4_STATE_WAIT;

  snprintf (uri, sizeof (uri),
      "%s%s",
      clientdata->remoteuri, action_str [clientdata->action]);

  clientdata->inuse = true;
  webrc = webclientGet (clientdata->webclient, uri);
  if (webrc != WEB_OK) {
    clientdata->inuse = false;
    return rc;
  }

  if (clientdata->state == BDJ4_STATE_PROCESS) {
    if (clientdata->webresplen > 0) {
      char    *tdata;
      char    *p;
      char    *tokstr = NULL;

      tdata = mdmalloc (clientdata->webresplen + 1);
      memcpy (tdata, clientdata->webresponse, clientdata->webresplen);
      tdata [clientdata->webresplen] = '\0';
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
    clientdata->state = BDJ4_STATE_OFF;
  }

  clientdata->inuse = false;
  return rc;
}


static bool
asbdj4GetAudioFile (asdata_t *asdata, const char *nm, const char *tempnm)
{
  bool              rc = false;
  int               webrc;
  char              uri [1024];
  char              query [MAXPATHLEN];
  int               clientkey;
  asclientdata_t    *clientdata;

  clientkey = asbdj4GetClientKeyByURI (asdata, nm);
  if (clientkey < 0) {
    return false;
  }
  clientdata = asdata->clientdata [clientkey];
  clientdata->action = ASBDJ4_ACT_GET_SONG;
  clientdata->state = BDJ4_STATE_WAIT;

  snprintf (uri, sizeof (uri),
      "%s%s",
      clientdata->remoteuri, action_str [clientdata->action]);
  snprintf (query, sizeof (query),
      "uri=%s",
      asbdj4StripPrefix (asdata, nm, clientkey));

  clientdata->inuse = true;
  webrc = webclientPost (clientdata->webclient, uri, query);
  if (webrc != WEB_OK) {
    clientdata->inuse = false;
    return rc;
  }

  if (clientdata->state == BDJ4_STATE_PROCESS) {
    if (clientdata->webresplen > 0) {
      FILE    *ofh;

      ofh = fileopOpen (tempnm, "wb");
      if (fwrite (clientdata->webresponse, clientdata->webresplen, 1, ofh) == 1) {
        rc = true;
      }
      fclose (ofh);
      mdextfclose (ofh);
    } else {
    }
    clientdata->state = BDJ4_STATE_OFF;
  }

  clientdata->inuse = false;
  return rc;
}

static int
asbdj4GetClientKeyByURI (asdata_t *asdata, const char *nm)
{
  int     count;
  int     askey = -1;

  count = ilistGetCount (asdata->urilist);
  for (int i = 0; i < count; ++i) {
    const char  *tstr;
    size_t      tlen;

    tstr = ilistGetStr (asdata->urilist, i, AS_CLIENT_URI);
    tlen = ilistGetNum (asdata->urilist, i, AS_CLIENT_URI_LEN);
    if (strncmp (nm, tstr, tlen) == 0) {
      askey = ilistGetNum (asdata->urilist, i, AS_CLIENT_ASKEY);
      break;
    }
  }

  if (askey < 0) {
    return -1;
  }

  return asbdj4GetClientKey (asdata, askey);
}

int
asbdj4GetClientKey (asdata_t *asdata, int askey)
{
  int   clientkey = -1;

  for (int i = 0; i < asdata->clientcount; ++i) {
    if (asdata->clientdata [i]->inuse == false &&
        asdata->clientdata [i]->askey == askey) {
      return i;
    }
  }

  clientkey = asbdj4ClientInit (asdata, askey);
  return clientkey;
}

static const char *
asbdj4StripPrefix (asdata_t *asdata, const char *songuri, int clientkey)
{
  const char  *turi;
  size_t      tlen;
  int         idx;

  idx = asdata->clientdata [clientkey]->uriidx;
  turi = ilistGetStr (asdata->urilist, idx, AS_CLIENT_URI);
  tlen = ilistGetNum (asdata->urilist, idx, AS_CLIENT_URI_LEN);
  if (strncmp (songuri, turi, tlen) == 0) {
    songuri += tlen;
  }
  return songuri;
}

static void
audiosrcClientFree (asdata_t *asdata)
{
  if (asdata->clientdata != NULL) {
    for (int i = 0; i < asdata->clientcount; ++i) {
      webclientClose (asdata->clientdata [i]->webclient);
      mdfree (asdata->clientdata [i]);
    }
    mdfree (asdata->clientdata);
  }
  asdata->clientdata = NULL;
  ilistFree (asdata->urilist);
  asdata->urilist = NULL;
  asdata->clientcount = 0;
}

static int
asbdj4ClientInit (asdata_t *asdata, int askey)
{
  int             idx;
  asclientdata_t  *clientdata;
  int             count;
  int             newclientcount;

  count = 0;
  while (atomic_flag_test_and_set (&asdata->locked)) {
    mssleep (30);
    ++count;
  }

  idx = asdata->clientcount;
  newclientcount = asdata->clientcount + 1;

  /* each clientdata pointer is allocated and must remain the same */
  /* so that the callback works when the array is extended */
  asdata->clientdata = mdrealloc (asdata->clientdata,
      sizeof (asclientdata_t *) * newclientcount);
  asdata->clientdata [idx] = mdmalloc (sizeof (asclientdata_t));
  clientdata = asdata->clientdata [idx];

  clientdata->askey = askey;
  clientdata->webclient = NULL;
  clientdata->inuse = false;
  clientdata->webresponse = NULL;
  clientdata->webresplen = 0;
  clientdata->uriidx = -1;
  clientdata->idx = idx;
  clientdata->remoteuri = NULL;
  clientdata->uri = NULL;
  clientdata->action = ASBDJ4_ACT_NONE;
  clientdata->state = BDJ4_STATE_OFF;
  clientdata->clientkey = idx;

  clientdata->webclient =
      webclientAlloc (clientdata, asbdj4WebResponseCallback);
  webclientIgnoreCertErr (clientdata->webclient);
  webclientSetUserPass (clientdata->webclient,
      asconfGetStr (asdata->asconf, askey, ASCONF_USER),
      asconfGetStr (asdata->asconf, askey, ASCONF_PASS));

  count = ilistGetCount (asdata->urilist);
  for (int i = 0; i < count; ++i) {
    int   taskey;

    taskey = ilistGetNum (asdata->urilist, i, AS_CLIENT_ASKEY);
    if (taskey == askey) {
      clientdata->uriidx = i;
      clientdata->remoteuri = ilistGetStr (asdata->urilist, i, AS_CLIENT_REMOTE_URI);
      clientdata->uri = ilistGetStr (asdata->urilist, i, AS_CLIENT_URI);
      break;
    }
  }

  asdata->clientcount = newclientcount;

  atomic_flag_clear (&asdata->locked);
  return idx;
}
