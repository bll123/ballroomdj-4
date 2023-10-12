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

#include "audioid.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
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

enum {
  ACOUSTID_BUFF_SZ = 16384,
};

typedef struct audioidacoustid {
  webclient_t   *webclient;
  const char    *webresponse;
  size_t        webresplen;
  mstime_t      globalreqtimer;
  int           respcount;
  char          key [100];
} audioidacoustid_t;

enum {
  QPS_LIMIT = 1000 / 3 + 1,
};

/*
 * acoustid:
 *  for each result (score)
 *    for each recording (recording-id)
 *      for each release
 *        : (album-artist, album-title, date, disctotal)
 *        medium: (discnumber, tracktotal)
 *        track: (tracknumber, title, artist)
 */

static audioidparse_t acoustidalbartistxp [] = {
  { AUDIOID_PARSE_DATA, AUDIOID_TYPE_JOINPHRASE, "/joinphrase", NULL, NULL, NULL },
  { AUDIOID_PARSE_DATA, TAG_ALBUMARTIST, "/name", NULL, NULL, NULL },
  { AUDIOID_PARSE_END,  AUDIOID_TYPE_TREE, "end-artist", NULL, NULL, NULL },
};

static audioidparse_t acoustidartistxp [] = {
  { AUDIOID_PARSE_DATA, AUDIOID_TYPE_JOINPHRASE, "/joinphrase", NULL, NULL, NULL },
  { AUDIOID_PARSE_DATA, TAG_ARTIST, "/name", NULL, NULL, NULL },
  { AUDIOID_PARSE_END,  AUDIOID_TYPE_TREE, "end-artist", NULL, NULL, NULL },
};

/* relative to /response/recordings/recording/releases/release/mediums/medium/tracks/track */
static audioidparse_t acoustidtrackxp [] = {
  { AUDIOID_PARSE_TREE, AUDIOID_TYPE_TREE, "/artists/artist", NULL, acoustidartistxp, NULL },
  { AUDIOID_PARSE_DATA, TAG_TRACKNUMBER, "/position", NULL, NULL, NULL },
  { AUDIOID_PARSE_DATA, TAG_TITLE, "/title", NULL, NULL, NULL },
  { AUDIOID_PARSE_END,  AUDIOID_TYPE_TREE, "end-track", NULL, NULL, NULL },
};

/* relative to /response/recordings/recording/releases/release/mediums/medium */
static audioidparse_t acoustidmediumxp [] = {
  { AUDIOID_PARSE_DATA, TAG_DISCNUMBER, "/position", NULL, NULL, NULL },
  { AUDIOID_PARSE_DATA, TAG_TRACKTOTAL, "/track_count", NULL, NULL, NULL },
  { AUDIOID_PARSE_TREE, AUDIOID_TYPE_TREE, "/tracks/track", NULL, acoustidtrackxp, NULL },
  { AUDIOID_PARSE_END,  AUDIOID_TYPE_TREE, "end-medium", NULL, NULL, NULL },
};

/* relative to /response/recordings/recording/releases/release */
static audioidparse_t acoustidreleasexp [] = {
  { AUDIOID_PARSE_TREE, AUDIOID_TYPE_TREE, "/artists/artist", NULL, acoustidalbartistxp, NULL },
  { AUDIOID_PARSE_DATA, TAG_ALBUM, "/title", NULL, NULL, NULL },
  { AUDIOID_PARSE_DATA, TAG_DATE, "/date/year", NULL, NULL, NULL },
  { AUDIOID_PARSE_DATA, AUDIOID_TYPE_MONTH, "/date/month", NULL, NULL, NULL },
  { AUDIOID_PARSE_DATA, TAG_DISCTOTAL, "/medium_count", NULL, NULL, NULL },
  { AUDIOID_PARSE_TREE, AUDIOID_TYPE_TREE, "/mediums/medium", NULL, acoustidmediumxp, NULL },
  { AUDIOID_PARSE_END,  AUDIOID_TYPE_TREE, "end-release", NULL, NULL, NULL },
};

/* relative to /response/recordings/recording */
static audioidparse_t acoustidrecordingxp [] = {
  { AUDIOID_PARSE_DATA, TAG_RECORDING_ID, "/id", NULL, NULL, NULL },
  { AUDIOID_PARSE_TREE, AUDIOID_TYPE_RESPIDX, "/releases/release", NULL, acoustidreleasexp, NULL },
  { AUDIOID_PARSE_END,  AUDIOID_TYPE_TREE, "end-recording", NULL, NULL, NULL },
};

/* relative to /response/results/result */
static audioidparse_t acoustidresultxp [] = {
  { AUDIOID_PARSE_DATA, TAG_AUDIOID_SCORE, "/score", NULL, NULL, NULL },
  { AUDIOID_PARSE_TREE, AUDIOID_TYPE_TREE, "/recordings/recording", NULL, acoustidrecordingxp, NULL },
  { AUDIOID_PARSE_END,  AUDIOID_TYPE_TREE, "end-result", NULL, NULL, NULL },
};

/* relative to /response/results */
static audioidparse_t acoustidresultsxp [] = {
  { AUDIOID_PARSE_DATA, AUDIOID_TYPE_STATUS_CODE, "/status", NULL, NULL, NULL },
  { AUDIOID_PARSE_TREE, AUDIOID_TYPE_TREE, "/result", NULL, acoustidresultxp, NULL },
  { AUDIOID_PARSE_END,  AUDIOID_TYPE_TREE, "end-results", NULL, NULL, NULL },
};

static audioidparse_t acoustidmainxp [] = {
  { AUDIOID_PARSE_TREE, AUDIOID_TYPE_TREE, "/response/results", NULL, acoustidresultsxp, NULL },
  { AUDIOID_PARSE_END,  AUDIOID_TYPE_TREE, "end-response", NULL, NULL, NULL },
};

static void acoustidWebResponseCallback (void *userdata, const char *resp, size_t len);
static void dumpData (audioidacoustid_t *acoustid);

audioidacoustid_t *
acoustidInit (void)
{
  audioidacoustid_t *acoustid;
  const char        *takey;

  acoustid = mdmalloc (sizeof (audioidacoustid_t));
  acoustid->webclient = webclientAlloc (acoustid, acoustidWebResponseCallback);
  webclientSetTimeout (acoustid->webclient, 15);
  /* if the bdj4 user-agent is used, cloudflare/acoustid will */
  /* return an http 503 error on certain queries (oddly specific). */
  /* so spoof a known user-agent instead */
  webclientSpoofUserAgent (acoustid->webclient);
  acoustid->webresponse = NULL;
  acoustid->webresplen = 0;
  mstimeset (&acoustid->globalreqtimer, 0);
  acoustid->respcount = 0;
  takey = bdjoptGetStr (OPT_G_ACOUSTID_KEY);
  vsencdec (takey, acoustid->key, sizeof (acoustid->key));

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

int
acoustidLookup (audioidacoustid_t *acoustid, const song_t *song,
    ilist_t *respdata)
{
  char          infn [MAXPATHLEN];
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
  mstime_t      starttm;
  int           webrc;

  fn = songGetStr (song, TAG_FILE);
  ffn = songutilFullFileName (fn);
  if (! fileopFileExists (ffn)) {
    return 0;
  }
  snprintf (infn, sizeof (infn), "%s%s",
      ffn, bdjvarsGetStr (BDJV_ORIGINAL_EXT));
  /* check for .original filename */
  if (! fileopFileExists (infn)) {
    strlcpy (infn, ffn, sizeof (infn));
  }
  mdfree (ffn);

  mstimestart (&starttm);
  /* acoustid prefers at most three calls per second */
  while (! mstimeCheck (&acoustid->globalreqtimer)) {
    mssleep (10);
  }
  mstimeset (&acoustid->globalreqtimer, QPS_LIMIT);
  logMsg (LOG_DBG, LOG_IMPORTANT, "acoustid: wait time: %" PRId64 "ms",
      (int64_t) mstimeend (&starttm));

  query = mdmalloc (ACOUSTID_BUFF_SZ);

  dur = songGetNum (song, TAG_DURATION);
  /* acoustid's fpcalc truncates the duration */
  ddur = floor ((double) dur / 1000.0);
  logMsg (LOG_DBG, LOG_AUDIO_ID, "acoustid: duration: %.0f", ddur);

  fpdata = mdmalloc (ACOUSTID_BUFF_SZ);

  targv [targc++] = sysvarsGetStr (SV_PATH_FPCALC);
  targv [targc++] = infn;
  targv [targc++] = "-plain";
  targv [targc++] = NULL;
  mstimestart (&starttm);
  osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_NOSTDERR,
      fpdata, ACOUSTID_BUFF_SZ, &retsz);
  logMsg (LOG_DBG, LOG_IMPORTANT, "acoustid: fp: %" PRId64 "ms",
      (int64_t) mstimeend (&starttm));

  strlcpy (uri, sysvarsGetStr (SV_AUDIOID_ACOUSTID_URI), sizeof (uri));
  /* if meta-compress is on the track artists are not generated */
  snprintf (query, ACOUSTID_BUFF_SZ,
      "client=%s"
      "&format=xml"
      "&meta=recordingids+recordings+releases+tracks+artists"
      "&duration=%.0f"
      "&fingerprint=%.*s",
      acoustid->key,
      ddur,
      (int) retsz, fpdata
      );

  mstimestart (&starttm);
  webrc = webclientPostCompressed (acoustid->webclient, uri, query);
  logMsg (LOG_DBG, LOG_IMPORTANT, "acoustid: web-query: %d %" PRId64 "ms",
      webrc, (int64_t) mstimeend (&starttm));
  if (webrc != WEB_OK) {
    return 0;
  }

  if (logCheck (LOG_DBG, LOG_AUDIOID_DUMP)) {
    dumpData (acoustid);
  }

  if (acoustid->webresponse != NULL && acoustid->webresplen > 0) {
    mstimestart (&starttm);
    acoustid->respcount = audioidParseXMLAll (acoustid->webresponse,
        acoustid->webresplen, acoustidmainxp, respdata, AUDIOID_ID_ACOUSTID);
    logMsg (LOG_DBG, LOG_IMPORTANT, "acoustid: parse: %" PRId64 "ms",
        (int64_t) mstimeend (&starttm));
  }

  dataFree (query);
  dataFree (fpdata);
  return acoustid->respcount;
}

static void
acoustidWebResponseCallback (void *userdata, const char *resp, size_t len)
{
  audioidacoustid_t   *acoustid = userdata;

  acoustid->webresponse = resp;
  acoustid->webresplen = len;
  return;
}

static void
dumpData (audioidacoustid_t *acoustid)
{
  FILE *ofh;

  if (acoustid->webresponse != NULL && acoustid->webresplen > 0) {
    ofh = fopen ("out-acoustid.xml", "w");
    if (ofh != NULL) {
      fwrite (acoustid->webresponse, 1, acoustid->webresplen, ofh);
      fprintf (ofh, "\n");
      fclose (ofh);
    }
  }
}

