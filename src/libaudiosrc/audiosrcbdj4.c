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
#include <ctype.h>
#include <errno.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "mdebug.h"
#include "pathinfo.h"
#include "playlist.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"
#include "webclient.h"

enum {
  ASBDJ4_ACT_NONE,
  ASBDJ4_ACT_EXISTS,
  ASBDJ4_ACT_GET,
  ASBDJ4_ACT_GET_PLAYLIST,
  ASBDJ4_ACT_GET_PL_NAMES,
  ASBDJ4_ACT_MAX,
};

enum {
  ASBDJ4_WAIT_MAX = 200,
};

const char *action_str [ASBDJ4_ACT_MAX] = {
  [ASBDJ4_ACT_NONE] = "none",
  [ASBDJ4_ACT_EXISTS] = "exists",
  [ASBDJ4_ACT_GET] = "get",
  [ASBDJ4_ACT_GET_PLAYLIST] = "getpl",
  [ASBDJ4_ACT_GET_PL_NAMES] = "plnames",
};

typedef struct asiterdata {
  const char      *dir;
  slist_t         *playlistnames;
  slistidx_t      plniter;
} asiterdata_t;

typedef struct asdata {
  webclient_t   *webclient;
  char          bdj4uri [MAXPATHLEN];
  const char    *musicdir;
  const char    *delpfx;
  const char    *origext;
  const char    *webresponse;
  size_t        musicdirlen;
  size_t        webresplen;
  int           action;
  int           state;
} asdata_t;

static void asbdj4MakeTempName (asdata_t *asdata, const char *ffn, char *tempnm, size_t maxlen);
static void asbdj4WebResponseCallback (void *userdata, const char *respstr, size_t len);

static long globalcount = 0;

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

bool
asiEnabled (void)
{
  const char  *srvuri;
  bool        enabled;

  srvuri = bdjoptGetStr (OPT_P_BDJ4_SERVER);
  enabled =
      (srvuri != NULL && *srvuri != '\0') &&
      bdjoptGetStr (OPT_P_BDJ4_SERVER_USER) != NULL &&
      bdjoptGetStr (OPT_P_BDJ4_SERVER_PASS) != NULL &&
      bdjoptGetNum (OPT_P_BDJ4_SERVER_PORT) >= 8000;
  return enabled;
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
  asdata->webclient = webclientAlloc (asdata, asbdj4WebResponseCallback);
  webclientSetTimeout (asdata->webclient, 5);
  snprintf (asdata->bdj4uri, sizeof (asdata->bdj4uri),
      "http://%s:%" PRIu16,
      bdjoptGetStr (OPT_P_BDJ4_SERVER),
      (uint16_t) bdjoptGetNum (OPT_P_BDJ4_SERVER_PORT));
  webclientSetUserPass (asdata->webclient,
      bdjoptGetStr (OPT_P_BDJ4_SERVER_USER),
      bdjoptGetStr (OPT_P_BDJ4_SERVER_PASS));
  asdata->action = ASBDJ4_ACT_NONE;
  asdata->state = BDJ4_STATE_OFF;
  return asdata;
}

void
asiPostInit (asdata_t *asdata, const char *musicdir)
{
  if (musicdir == NULL) {
    return;
  }
  asdata->musicdir = musicdir;
  asdata->musicdirlen = strlen (asdata->musicdir);
}

void
asiFree (asdata_t *asdata)
{
  if (asdata == NULL) {
    return;
  }

  webclientClose (asdata->webclient);
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
  int     count;
  int     webrc;
  char    query [1024];

  asdata->action = ASBDJ4_ACT_EXISTS;
  asdata->state = BDJ4_STATE_WAIT;
  snprintf (query, sizeof (query),
      "action=%s"
      "&uri=%s",
      action_str [asdata->action], nm);

  webrc = webclientPost (asdata->webclient, asdata->bdj4uri, query);
  if (webrc != WEB_OK) {
    return exists;
  }

  count = 0;
  while (asdata->state == BDJ4_STATE_WAIT && count < ASBDJ4_WAIT_MAX) {
    mssleep (10);
    ++count;
  }

  if (asdata->state == BDJ4_STATE_PROCESS) {
    if (asdata->webresplen > 0 &&
        strncmp (asdata->webresponse, "T", 1) == 0) {
      exists = true;
    }
    asdata->state = BDJ4_STATE_OFF;
  }

  return exists;
}

bool
asiPrep (asdata_t *asdata, const char *sfname, char *tempnm, size_t sz)
{
  char      ffn [MAXPATHLEN];
  mstime_t  mstm;
  char      *buff;
  size_t    frc;
  ssize_t   fsz;
  time_t    tm;
  FILE      *fh;

  if (sfname == NULL || tempnm == NULL) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "WARN: prep: null-data");
    return false;
  }

  mstimestart (&mstm);
  asiFullPath (asdata, sfname, ffn, sizeof (ffn), NULL, 0);
  asbdj4MakeTempName (asdata, ffn, tempnm, sz);

  /* VLC still cannot handle internationalized names. */
  /* I wonder how they handle them internally. */
  /* Symlinks work on Linux/Mac OS. */
  fileopDelete (tempnm);
  filemanipLinkCopy (ffn, tempnm);
  if (! fileopFileExists (tempnm)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file copy failed: %s", tempnm);
    return false;
  }

  /* read the entire file in order to get it into the operating system's */
  /* filesystem cache */
  fsz = fileopSize (ffn);
  if (fsz <= 0) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file size 0: %s", ffn);
    return false;
  }
  buff = mdmalloc (fsz);
  fh = fileopOpen (ffn, "rb");
  frc = fread (buff, fsz, 1, fh);
  mdextfclose (fh);
  fclose (fh);
  mdfree (buff);
  if (frc != 1) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: file read failed: %s", tempnm);
    return false;
  }

  tm = mstimeend (&mstm);
  logMsg (LOG_DBG, LOG_BASIC, "prep-time (%" PRIu64 ") %s", (uint64_t) tm, sfname);

  return true;
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

//  stpecpy (buff, buff + sz, sfname);
//  stpecpy (buff, buff + sz, AS_BDJ4_PFX);
  asiFullPath (asdata, sfname, buff + AS_BDJ4_PFX_LEN, sz - AS_BDJ4_PFX_LEN,
      prefix, pfxlen);
}

void
asiFullPath (asdata_t *asdata, const char *sfname, char *buff, size_t sz,
    const char *prefix, int pfxlen)
{
  *buff = '\0';

  if (sfname == NULL || buff == NULL) {
    return;
  }

  if (strncmp (sfname, AS_BDJ4_PFX, AS_BDJ4_PFX_LEN) == 0) {
    sfname += AS_BDJ4_PFX_LEN;
  }
}

const char *
asiRelativePath (asdata_t *asdata, const char *sfname, int pfxlen)
{
  const char  *p = sfname;

  if (sfname == NULL) {
    return NULL;
  }

  if (strncmp (p, AS_BDJ4_PFX, AS_BDJ4_PFX_LEN) == 0) {
    p += AS_BDJ4_PFX_LEN;
  }

  return p;
}

size_t
asiDir (asdata_t *asdata, const char *sfname, char *buff, size_t sz, int pfxlen)
{
  size_t    rc = 0;

  *buff = '\0';

  if (sfname == NULL) {
    return rc;
  }

  if (pfxlen > 0) {
    snprintf (buff, sz, "%.*s", pfxlen, sfname);
    rc = pfxlen;
  } else {
    snprintf (buff, sz, "%s", asdata->musicdir);
    rc = asdata->musicdirlen;
  }

  return rc;
}

asiterdata_t *
asiStartIterator (asdata_t *asdata, const char *dir)
{
  asiterdata_t  *asidata;

  if (dir == NULL) {
    return NULL;
  }

  if (! fileopIsDirectory (dir)) {
    return NULL;
  }

  asidata = mdmalloc (sizeof (asiterdata_t));
  asidata->dir = dir;

//  asidata->playlistnames = dirlistRecursiveDirList (dir, DIRLIST_FILES);
  slistStartIterator (asidata->playlistnames, &asidata->plniter);

  return asidata;
}

void
asiCleanIterator (asdata_t *asdata, asiterdata_t *asidata)
{
  if (asidata == NULL) {
    return;
  }

  if (asidata->playlistnames != NULL) {
    slistFree (asidata->playlistnames);
  }
  mdfree (asidata);
}

int32_t
asiIterCount (asdata_t *asdata, asiterdata_t *asidata)
{
  int32_t   c = 0;

  if (asidata == NULL) {
    return c;
  }

  c = slistGetCount (asidata->playlistnames);
  return c;
}

const char *
asiIterate (asdata_t *asdata, asiterdata_t *asidata)
{
  const char    *rval = NULL;

  if (asidata == NULL) {
    return NULL;
  }

  rval = slistIterateKey (asidata->playlistnames, &asidata->plniter);
  return rval;
}

bool
asiGetPlaylistNames (asdata_t *asdata)
{
  bool    rc = false;
  int     webrc;
  char    query [1024];

fprintf (stderr, "asi-gpln: begin\n");
  asdata->action = ASBDJ4_ACT_GET_PL_NAMES;
  asdata->state = BDJ4_STATE_WAIT;
  snprintf (query, sizeof (query),
      "action=%s",
      action_str [asdata->action]);

  webrc = webclientPost (asdata->webclient, asdata->bdj4uri, query);
  if (webrc != WEB_OK) {
    return rc;
  }

  if (asdata->state == BDJ4_STATE_PROCESS) {
    if (asdata->webresplen > 0 &&
        strncmp (asdata->webresponse, "T", 1) == 0) {
      rc = true;
    }
    asdata->state = BDJ4_STATE_OFF;
  }

  return rc;
}

/* internal routines */

static void
asbdj4MakeTempName (asdata_t *asdata, const char *ffn, char *tempnm, size_t maxlen)
{
  char        tnm [MAXPATHLEN];
  size_t      idx;
  pathinfo_t  *pi;

  pi = pathInfo (ffn);

  idx = 0;
  for (const char *p = pi->filename;
      *p && idx < maxlen && idx < pi->flen; ++p) {
    if ((isascii (*p) && isalnum (*p)) ||
        *p == '.' || *p == '-' || *p == '_') {
      tnm [idx++] = *p;
    }
  }
  tnm [idx] = '\0';
  pathInfoFree (pi);

  /* the profile index so we don't stomp on other bdj instances   */
  /* the global count so we don't stomp on ourselves              */
  snprintf (tempnm, maxlen, "tmp/%02" PRId64 "-%03ld-%s",
      sysvarsGetNum (SVL_PROFILE_IDX), globalcount, tnm);
  ++globalcount;
}

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
