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

#include "audioid.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "webclient.h"

typedef struct audioidmb {
  webclient_t   *webclient;
  const char    *webresponse;
  size_t        webresplen;
  mstime_t      globalreqtimer;
  int           respcount;
} audioidmb_t;


/*
 * musicbrainz:
 *  for each result (score=100.0)
 *    for each recording (title, length, artist, work-id)
 *      for each release in the release-list
 *      : (album-title, album-artist, date)
 *      medium-list : (disc-total)
 *      medium : (disc-number)
 *      medium/track-list: (track-total)
 *      medium/track-list/track : (track-number, title)
 */

static audioidxpath_t mbartistxp [] = {
  { AUDIOID_XPATH_DATA,  TAG_ARTIST, "/artist/name", NULL, NULL },
  { AUDIOID_XPATH_END,   AUDIOID_TYPE_TREE, "end-artist", NULL, NULL },
};

static audioidxpath_t mbalbartistxp [] = {
  { AUDIOID_XPATH_DATA,  TAG_ALBUMARTIST, "/artist/name", NULL, NULL },
  { AUDIOID_XPATH_END,   AUDIOID_TYPE_TREE, "end-artist", NULL, NULL },
};

/* relative to /metadata/recording/release-list/release/medium */
static audioidxpath_t mbmediumxp [] = {
  { AUDIOID_XPATH_DATA,  TAG_DISCNUMBER, "/position", NULL, NULL },
  { AUDIOID_XPATH_DATA,  TAG_TRACKTOTAL, "/track-list", "count", NULL },
  { AUDIOID_XPATH_DATA,  TAG_TRACKNUMBER, "/track-list/track/position", NULL, NULL },
  { AUDIOID_XPATH_DATA,  TAG_TITLE, "/track-list/track/title", NULL, NULL },
  { AUDIOID_XPATH_DATA,  TAG_DURATION, "/track-list/track/length", NULL, NULL },
  { AUDIOID_XPATH_END,   AUDIOID_TYPE_TREE, "end-medium", NULL, NULL },
};

/* relative to /metadata/recording/release-list/release */
static audioidxpath_t mbreleasexp [] = {
  { AUDIOID_XPATH_DATA,  TAG_ALBUM, "/title", NULL, NULL },
  { AUDIOID_XPATH_DATA,  TAG_DATE, "/date", NULL, NULL },
  { AUDIOID_XPATH_TREE,  AUDIOID_TYPE_JOINPHRASE, "/artist-credit/name-credit", "joinphrase", mbalbartistxp  },
  { AUDIOID_XPATH_TREE,  AUDIOID_TYPE_TREE, "/medium-list/medium", NULL, mbmediumxp },
  { AUDIOID_XPATH_END,   AUDIOID_TYPE_TREE, "end-release", NULL, NULL },
};

/* relative to /metadata/recording */
static audioidxpath_t mbrecordingxp [] = {
  { AUDIOID_XPATH_DATA,  TAG_TITLE, "/title", NULL, NULL },
  { AUDIOID_XPATH_DATA,  TAG_DURATION, "/length", NULL, NULL },
  { AUDIOID_XPATH_DATA,  TAG_WORK_ID, "/relation-list/relation/target", NULL, NULL },
  { AUDIOID_XPATH_TREE,  AUDIOID_TYPE_JOINPHRASE, "/artist-credit/name-credit", "joinphrase", mbartistxp },
  { AUDIOID_XPATH_TREE,  AUDIOID_TYPE_RESPIDX, "/release-list/release", NULL, mbreleasexp },
  { AUDIOID_XPATH_END,   AUDIOID_TYPE_TREE, "end-recording", NULL, NULL },
};

static audioidxpath_t mbxpaths [] = {
  { AUDIOID_XPATH_TREE,  AUDIOID_TYPE_TREE, "/metadata/recording", NULL, mbrecordingxp },
  { AUDIOID_XPATH_END,   AUDIOID_TYPE_TREE, "end-metadata", NULL, NULL },
};

static void mbWebResponseCallback (void *userdata, const char *resp, size_t len);
static void dumpData (audioidmb_t *mb);

audioidmb_t *
mbInit (void)
{
  audioidmb_t *mb;

  mb = mdmalloc (sizeof (audioidmb_t));
  mb->webclient = webclientAlloc (mb, mbWebResponseCallback);
  mb->webresponse = NULL;
  mb->webresplen = 0;
  mstimeset (&mb->globalreqtimer, 0);

  return mb;
}

void
mbFree (audioidmb_t *mb)
{
  if (mb == NULL) {
    return;
  }

  webclientClose (mb->webclient);
  mb->webclient = NULL;
  mb->webresponse = NULL;
  mb->webresplen = 0;
  mb->respcount = 0;
  mdfree (mb);
}

int
mbRecordingIdLookup (audioidmb_t *mb, const char *recid, ilist_t *respdata)
{
  char          uri [MAXPATHLEN];
  mstime_t      starttm;
  int           webrc;

  /* musicbrainz prefers only one call per second */
  mstimestart (&starttm);
  while (! mstimeCheck (&mb->globalreqtimer)) {
    mssleep (10);
  }
  mstimeset (&mb->globalreqtimer, 1000);
  logMsg (LOG_DBG, LOG_IMPORTANT, "mb: wait time: %" PRId64 "ms",
      (int64_t) mstimeend (&starttm));

  strlcpy (uri, sysvarsGetStr (SV_AUDIOID_MUSICBRAINZ_URI), sizeof (uri));
  strlcat (uri, "/recording/", sizeof (uri));
  strlcat (uri, recid, sizeof (uri));
  /* artist-credits retrieves the additional artists for the song */
  /* media is needed to get the track number and track total */
  /* work-rels is needed to get the work-id */
  /* releases is used to get the list of releases for this recording */
  /*    a match can then possibly be made by album name/track number */
  /* releases is used to get the list of releases for this recording */
  /* musicbrainz limits the number of responses to 25, and there appears */
  /* to be many duplicates.  I could use better filtering. */
  /* for this reason, musicbrainz is not used as the primary source. */
  strlcat (uri, "?inc=artist-credits+work-rels+releases+artists+media+isrcs", sizeof (uri));
  logMsg (LOG_DBG, LOG_AUDIO_ID, "audioid: mb: uri: %s", uri);

  mstimestart (&starttm);
  webrc = webclientGet (mb->webclient, uri);
  logMsg (LOG_DBG, LOG_IMPORTANT, "mb: web-query: %d %" PRId64 "ms",
      webrc, (int64_t) mstimeend (&starttm));
  if (webrc != WEB_OK) {
    return 0;
  }

  if (logCheck (LOG_DBG, LOG_AUDIOID_DUMP)) {
    dumpData (mb);
  }

  if (mb->webresponse != NULL && mb->webresplen > 0) {
    mstimestart (&starttm);
    mb->respcount = audioidParseAll (mb->webresponse, mb->webresplen,
        mbxpaths, respdata, AUDIOID_ID_MB_LOOKUP);
    logMsg (LOG_DBG, LOG_IMPORTANT, "mb: parse: %" PRId64 "ms",
        (int64_t) mstimeend (&starttm));
  }

  return mb->respcount;
}

/* internal routines */

static void
mbWebResponseCallback (void *userdata, const char *resp, size_t len)
{
  audioidmb_t   *mb = userdata;

  mb->webresponse = resp;
  mb->webresplen = len;
  return;
}


static void
dumpData (audioidmb_t *mb)
{
  FILE *ofh;

  if (mb->webresponse != NULL && mb->webresplen > 0) {
    ofh = fopen ("out-mb.xml", "w");
    if (ofh != NULL) {
      fwrite (mb->webresponse, 1, mb->webresplen, ofh);
      fprintf (ofh, "\n");
      fclose (ofh);
    }
  }
}
