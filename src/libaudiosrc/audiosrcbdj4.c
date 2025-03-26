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
  ASBDJ4_ACT_NONE,
  ASBDJ4_ACT_GET_PL_NAMES,
  ASBDJ4_ACT_GET_PLAYLIST,
  ASBDJ4_ACT_SONG_EXISTS,
  ASBDJ4_ACT_GET_SONG,
  ASBDJ4_ACT_SONG_TAGS,
  ASBDJ4_ACT_MAX,
};

enum {
  ASBDJ4_WAIT_MAX = 200,
  ASBDJ4_CLIENT_MAX = 3,
};

const char *action_str [ASBDJ4_ACT_MAX] = {
  [ASBDJ4_ACT_NONE] = "none",
  [ASBDJ4_ACT_SONG_EXISTS] = "songexists",
  [ASBDJ4_ACT_GET_SONG] = "songget",
  [ASBDJ4_ACT_SONG_TAGS] = "songtags",
  [ASBDJ4_ACT_GET_PLAYLIST] = "plget",
  [ASBDJ4_ACT_GET_PL_NAMES] = "plnames",
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
  char          **bdj4uri;
  char          **clienturi;
  int           *clienturilen;
  int           *clientaskey;
  const char    *musicdir;
  const char    *delpfx;
  const char    *origext;
  const char    *webresponse;
  size_t        musicdirlen;
  size_t        webresplen;
  int           clientcount;
  int           action;
  int           state;
} asdata_t;

static void asbdj4WebResponseCallback (void *userdata, const char *respstr, size_t len);
static bool asbdj4GetPlaylist (asdata_t *asdata, asiterdata_t *asidata, const char *nm);
static bool asbdj4SongTags (asdata_t *asdata, asiterdata_t *asidata, const char *songuri);
static bool asbdj4GetPlaylistNames (asdata_t *asdata, asiterdata_t *asidata, int askey);
static bool asbdj4GetAudioFile (asdata_t *asdata, const char *nm, const char *tempnm);
static int asbdj4GetClientKey (asdata_t *asdata, const char *nm);

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
  asdata_t    *asdata;

  asdata = mdmalloc (sizeof (asdata_t));
  asdata->musicdir = "";
  asdata->musicdirlen = 0;
  asdata->delpfx = delpfx;
  asdata->origext = origext;

  asdata->asconf = asconfAlloc ();
  asdata->webclient = NULL;
  asdata->clientcount = 0;
  asdata->clienturi = NULL;
  asdata->clienturilen = NULL;
  asdata->clientaskey = NULL;
  asdata->bdj4uri = NULL;
  asdata->action = ASBDJ4_ACT_NONE;
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
      if (type == AUDIOSRC_TYPE_BDJ4) {
        ++count;
      }
    }
  }

  asdata->clientcount = count;
  asdata->webclient = mdmalloc (sizeof (webclient_t *) * count);
  asdata->bdj4uri = mdmalloc (sizeof (char *) * count);
  asdata->clienturi = mdmalloc (sizeof (char *) * count);
  asdata->clienturilen = mdmalloc (sizeof (int) * count);
  asdata->clientaskey = mdmalloc (sizeof (int) * count);

  count = 0;
  asconfStartIterator (asdata->asconf, &iteridx);
  while ((askey = asconfIterate (asdata->asconf, &iteridx)) >= 0) {
    if (asconfGetNum (asdata->asconf, askey, ASCONF_MODE) == ASCONF_MODE_CLIENT) {
      int   type;

      type = asconfGetNum (asdata->asconf, askey, ASCONF_TYPE);
      if (type == AUDIOSRC_TYPE_BDJ4) {
        asdata->webclient [count] = webclientAlloc (asdata, asbdj4WebResponseCallback);
        webclientSetTimeout (asdata->webclient [count], 1);
        snprintf (temp, sizeof (temp),
            "http://%s:%" PRIu16,
            asconfGetStr (asdata->asconf, askey, ASCONF_URI),
            (uint16_t) asconfGetNum (asdata->asconf, askey, ASCONF_PORT));
        asdata->bdj4uri [count] = mdstrdup (temp);
        snprintf (temp, sizeof (temp),
            "%s%s:%" PRIu16, AS_BDJ4_PFX,
            asconfGetStr (asdata->asconf, askey, ASCONF_URI),
            (uint16_t) asconfGetNum (asdata->asconf, askey, ASCONF_PORT));
        asdata->clienturi [count] = mdstrdup (temp);
        asdata->clienturilen [count] = strlen (temp);
        asdata->clientaskey [count] = askey;
        webclientSetUserPass (asdata->webclient [count],
            asconfGetStr (asdata->asconf, askey, ASCONF_USER),
            asconfGetStr (asdata->asconf, askey, ASCONF_PASS));
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

  for (int i = 0; i < asdata->clientcount; ++i) {
    webclientClose (asdata->webclient [i]);
    mdfree (asdata->bdj4uri [i]);
    mdfree (asdata->clienturi [i]);
  }
  mdfree (asdata->webclient);
  mdfree (asdata->bdj4uri);
  mdfree (asdata->clienturi);
  mdfree (asdata->clienturilen);
  mdfree (asdata->clientaskey);
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
asiExists (asdata_t *asdata, const char *nm)
{
  bool    exists = false;
  int     webrc;
  char    query [1024];
  int     clientkey;

  asdata->action = ASBDJ4_ACT_SONG_EXISTS;
  asdata->state = BDJ4_STATE_WAIT;
  clientkey = asbdj4GetClientKey (asdata, nm);
  if (clientkey < 0) {
    return false;
  }

  snprintf (query, sizeof (query),
      "%s/%s"
      "?uri=%s",
      asdata->bdj4uri [clientkey], action_str [asdata->action], nm);

  webrc = webclientGet (asdata->webclient [clientkey], query);
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
  mstime_t  mstm;
  time_t    tm;
  int       rc = false;

  if (sfname == NULL || tempnm == NULL) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "WARN: prep: null-data");
    return false;
  }

  mstimestart (&mstm);
  audiosrcMakeTempName (sfname, tempnm, sz);

  fileopDelete (tempnm);

  rc = asbdj4GetAudioFile (asdata, sfname, tempnm);
  /* the file should already be in the disk cache as it was just written */

  if (! fileopFileExists (tempnm)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file copy failed: %s", tempnm);
    return false;
  }

  tm = mstimeend (&mstm);
  logMsg (LOG_DBG, LOG_BASIC, "prep-time (%" PRIu64 ") %s", (uint64_t) tm, sfname);

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
asiStartIterator (asdata_t *asdata, asitertype_t asitertype, const char *nm, int askey)
{
  asiterdata_t  *asidata;

  asidata = mdmalloc (sizeof (asiterdata_t));
  asidata->songlist = NULL;
  asidata->songtags = NULL;
  asidata->iterlist = NULL;

  if (asitertype == AS_ITER_PL_NAMES) {
    asbdj4GetPlaylistNames (asdata, asidata, askey);
    asidata->iterlist = asidata->plNames;
    slistStartIterator (asidata->iterlist, &asidata->iteridx);
  } else if (asitertype == AS_ITER_PL) {
    asbdj4GetPlaylist (asdata, asidata, nm);
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
asbdj4WebResponseCallback (void *userdata, const char *respstr, size_t len)
{
  asdata_t    *asdata = (asdata_t *) userdata;

  if (asdata->action == ASBDJ4_ACT_NONE) {
    asdata->state = BDJ4_STATE_OFF;
    return;
  }

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
asbdj4GetPlaylist (asdata_t *asdata, asiterdata_t *asidata, const char *nm)
{
  bool    rc = false;
  int     webrc;
  char    query [1024];
  int     clientkey;

  asdata->action = ASBDJ4_ACT_GET_PLAYLIST;
  asdata->state = BDJ4_STATE_WAIT;
  clientkey = asbdj4GetClientKey (asdata, nm);
  if (clientkey < 0) {
    return false;
  }

  snprintf (query, sizeof (query),
      "%s/%s"
      "?uri=%s",
      asdata->bdj4uri [clientkey], action_str [asdata->action],
      nm);

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

      p = strtok_r (tdata, MSG_ARGS_RS_STR, &tokstr);
      while (p != NULL) {
        slistSetNum (asidata->songlist, p, 1);
        p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
      }

      rc = true;
    }
    asdata->state = BDJ4_STATE_OFF;
  }

  return rc;
}

static bool
asbdj4SongTags (asdata_t *asdata, asiterdata_t *asidata, const char *songuri)
{
  bool    rc = false;
  int     webrc;
  char    query [1024];
  int     clientkey;

  asdata->action = ASBDJ4_ACT_SONG_TAGS;
  asdata->state = BDJ4_STATE_WAIT;
  clientkey = asbdj4GetClientKey (asdata, songuri);
  if (clientkey < 0) {
    return false;
  }

  snprintf (query, sizeof (query),
      "%s/%s"
      "?uri=%s",
      asdata->bdj4uri [clientkey], action_str [asdata->action],
      songuri);

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

  return rc;
}

static bool
asbdj4GetPlaylistNames (asdata_t *asdata, asiterdata_t *asidata, int askey)
{
  bool    rc = false;
  int     webrc;
  char    query [1024];
  int     clientkey;

  for (int i = 0; i < asdata->clientcount; ++i) {
    if (asdata->clientaskey [i] == askey) {
      clientkey = i;
      break;
    }
  }

  asdata->action = ASBDJ4_ACT_GET_PL_NAMES;
  asdata->state = BDJ4_STATE_WAIT;
  snprintf (query, sizeof (query),
      "%s/%s",
      asdata->bdj4uri [clientkey], action_str [asdata->action]);

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
        /* CONTEXT: the name of the history song list */
        if (strcmp (p, _("History")) == 0) {
          p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
          continue;
        }
        slistSetNum (asidata->plNames, p, 1);
        p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
      }
      slistSort (asidata->plNames);
      rc = true;
    }
    asdata->state = BDJ4_STATE_OFF;
  }

  return rc;
}


static bool
asbdj4GetAudioFile (asdata_t *asdata, const char *nm, const char *tempnm)
{
  bool    rc = false;
  int     webrc;
  char    query [1024];
  int     clientkey;

  asdata->action = ASBDJ4_ACT_GET_SONG;
  asdata->state = BDJ4_STATE_WAIT;
  clientkey = asbdj4GetClientKey (asdata, nm);
  if (clientkey < 0) {
    return false;
  }

  snprintf (query, sizeof (query),
      "%s/%s"
      "?uri=%s",
      asdata->bdj4uri [clientkey], action_str [asdata->action],
      nm);

  webrc = webclientGet (asdata->webclient [clientkey], query);
  if (webrc != WEB_OK) {
    return rc;
  }

  if (asdata->state == BDJ4_STATE_PROCESS) {
    if (asdata->webresplen > 0) {
      FILE    *ofh;

      ofh = fileopOpen (tempnm, "wb");
      if (fwrite (asdata->webresponse, asdata->webresplen, 1, ofh) == 1) {
        rc = true;
      }
      fclose (ofh);
      mdextfclose (ofh);
    }
    asdata->state = BDJ4_STATE_OFF;
  }

  return rc;
}

static int
asbdj4GetClientKey (asdata_t *asdata, const char *nm)
{
  int     clientkey = -1;

  for (int i = 0; i < asdata->clientcount; ++i) {
    if (strncmp (nm, asdata->clienturi [i], asdata->clienturilen [i]) == 0) {
      clientkey = i;
      break;
    }
  }

  return clientkey;
}
