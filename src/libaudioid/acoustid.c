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
#include <math.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "audioid.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "osprocess.h"
#include "slist.h"
#include "song.h"
#include "songutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "vsencdec.h"
#include "webclient.h"

typedef struct audioidacoustid {
  webclient_t   *webclient;
  const char    *webresponse;
  size_t        webresplen;
  mstime_t      globalreqtimer;
  int           respcount;
  char          akey [100];
} audioidacoustid_t;

typedef struct {
  int             flag;
  int             tagidx;
  const char      *xpath;
  const char      *attr;
} acoustidxpath_t;

enum {
  ACOUSTID_SINGLE,
  ACOUSTID_RELEASE,
  ACOUSTID_SKIP,
};

enum {
  ACOUSTID_TYPE_JOINPHRASE = TAG_KEY_MAX + 1,
  ACOUSTID_TYPE_RELCOUNT = TAG_KEY_MAX + 2,
  ACOUSTID_BUFF_SZ = 16384,
};

// relative to /metadata/recording/release-list
static const char *acoustidreleasexpath = "/release";
static acoustidxpath_t acoustidxpaths [] = {
  { ACOUSTID_SINGLE,  TAG_TITLE, "/metadata/recording/title", NULL },
  { ACOUSTID_SINGLE,  TAG_DURATION, "/metadata/recording/length", NULL },
  { ACOUSTID_SINGLE,  TAG_WORK_ID, "/metadata/recording/relation-list/relation/target", NULL },
  // joinphrase must appear before artist
  { ACOUSTID_SKIP,    ACOUSTID_TYPE_JOINPHRASE, "/metadata/recording/artist-credit/name-credit", "joinphrase" },
  { ACOUSTID_SINGLE,  TAG_ARTIST, "/metadata/recording/artist-credit/name-credit/artist/name", NULL },
  // relcount must appear before the release xpaths
  { ACOUSTID_SKIP,    ACOUSTID_TYPE_RELCOUNT, "/metadata/recording/release-list", "count" },
  // relative to /metadata/recording/release-list/release
  { ACOUSTID_RELEASE, TAG_ALBUM, "/title", NULL },
  { ACOUSTID_RELEASE, TAG_DATE, "/date", NULL },
  { ACOUSTID_RELEASE, TAG_ALBUMARTIST, "/artist-credit/name-credit/artist/name", NULL },
  { ACOUSTID_RELEASE, TAG_DISCNUMBER, "/medium-list/medium/position", NULL },
  { ACOUSTID_RELEASE, TAG_TRACKNUMBER, "/medium-list/medium/track-list/track/position", NULL },
  { ACOUSTID_RELEASE, TAG_TRACKTOTAL, "/medium-list/medium/track-list", "count" },
};
enum {
  acoustidxpathcount = sizeof (acoustidxpaths) / sizeof (acoustidxpath_t),
};

static void acoustidWebResponseCallback (void *userdata, const char *resp, size_t len);

audioidacoustid_t *
acoustidInit (void)
{
  audioidacoustid_t *acoustid;
  const char        *takey;

  acoustid = mdmalloc (sizeof (audioidacoustid_t));
  acoustid->webclient = webclientAlloc (acoustid, acoustidWebResponseCallback);
  acoustid->webresponse = NULL;
  acoustid->webresplen = 0;
  mstimeset (&acoustid->globalreqtimer, 0);
  acoustid->respcount = 0;
  takey = bdjoptGetStr (OPT_G_ACOUSTID_KEY);
  vsencdec (takey, acoustid->akey, sizeof (acoustid->akey));

  return acoustid;
}

void
acoustidFree (audioidacoustid_t *acoustid)
{
  if (acoustid == NULL) {
    return;
  }

  webclientClose (acoustid->webclient);
  acoustid->webclient = NULL;
  acoustid->webresponse = NULL;
  acoustid->webresplen = 0;
  mdfree (acoustid);
}

void
acoustidLookup (audioidacoustid_t *acoustid, const song_t *song,
    ilist_t *respdata)
{
  char          uri [MAXPATHLEN];
  char          *query;
  long          dur;
  double        ddur;
  char          *fpdata = NULL;
  size_t        retsz;
  const char    *fn;
  char          *ffn;
  const char    *targv [10];
  int           targc = 0;

  fn = songGetStr (song, TAG_FILE);
  ffn = songutilFullFileName (fn);
  if (! fileopFileExists (ffn)) {
    return;
  }

  /* acoustid prefers at most three calls per second */
  while (! mstimeCheck (&acoustid->globalreqtimer)) {
    mssleep (10);
  }
  mstimeset (&acoustid->globalreqtimer, 334);

  query = mdmalloc (ACOUSTID_BUFF_SZ);

  dur = songGetNum (song, TAG_DURATION);
  ddur = round ((double) dur / 1000.0);

  fpdata = mdmalloc (ACOUSTID_BUFF_SZ);
  targv [targc++] = sysvarsGetStr (SV_PATH_FPCALC);
  targv [targc++] = ffn;
  targv [targc++] = "-plain";
  targv [targc++] = NULL;
  osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_NOSTDERR,
      fpdata, ACOUSTID_BUFF_SZ, &retsz);

  strlcpy (uri, sysvarsGetStr (SV_AUDIOID_ACOUSTID_URI), sizeof (uri));
  snprintf (query, ACOUSTID_BUFF_SZ,
      "client=%s"
      "&format=xml"
      "&meta=recordingids+recordings+releases+tracks+compress"
      "&duration=%.0f"
      "&fingerprint=%.*s",
      acoustid->akey,
      ddur,
      (int) retsz, fpdata
      );
  logMsg (LOG_DBG, LOG_AUDIO_ID, "audioid: acoustid: query: %s", query);

  webclientPostCompressed (acoustid->webclient, uri, query);
{
FILE *ofh;
ofh = fopen ("out.xml", "w");
fwrite (acoustid->webresponse, 1, acoustid->webresplen, ofh);
fprintf (ofh, "\n");
fclose (ofh);
}

  dataFree (query);
  dataFree (fpdata);
  return;
}

static void
acoustidWebResponseCallback (void *userdata, const char *resp, size_t len)
{
  audioidacoustid_t   *acoustid = userdata;

  acoustid->webresponse = resp;
  acoustid->webresplen = len;
  return;
}
