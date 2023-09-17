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

#include "filedata.h"     // temporary, remove

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

static audioidxpath_t mbxpaths [] = {
  { AUDIOID_XPATH_TREE,     AUDIOID_TYPE_TREE, "/metadata", NULL },
  { AUDIOID_XPATH_TREE,     AUDIOID_TYPE_TREE, "/recording", NULL },
  { AUDIOID_XPATH_DATA,     TAG_TITLE, "/title", NULL },
  { AUDIOID_XPATH_DATA,     TAG_DURATION, "/length", NULL },
  { AUDIOID_XPATH_DATA,     TAG_WORK_ID, "/relation-list/relation/target", NULL },
  { AUDIOID_XPATH_TREE,     AUDIOID_TYPE_JOINPHRASE, "/artist-credit/name-credit", "joinphrase" },
  { AUDIOID_XPATH_DATA,     TAG_ARTIST, "/artist/name", NULL },
  { AUDIOID_XPATH_END_TREE, AUDIOID_TYPE_TREE, NULL, NULL },
  { AUDIOID_XPATH_TREE,     AUDIOID_TYPE_RESPONSE, "/release-list/release", NULL },
  /* relative to /metadata/recording/release-list/release */
  /* "medium-list: count" is the count of the number of mediums, not */
  /* the disc-total :( */
  /* disc-total does not seem to be present, though the data must be */
  /* somewhere, as acoustid has it */
  { AUDIOID_XPATH_DATA,     TAG_ALBUM, "/title", NULL },
  { AUDIOID_XPATH_DATA,     TAG_DATE, "/date", NULL },
  { AUDIOID_XPATH_TREE,     AUDIOID_TYPE_JOINPHRASE, "/artist-credit/name-credit", "joinphrase" },
  { AUDIOID_XPATH_DATA,     TAG_ALBUMARTIST, "/artist-credit/name-credit/artist/name", NULL },
  { AUDIOID_XPATH_END_TREE, AUDIOID_TYPE_TREE, NULL, NULL },
  { AUDIOID_XPATH_TREE,     AUDIOID_TYPE_TREE, "/medium-list/medium", NULL },
  /* relative to /metadata/recording/release-list/release/medium */
  { AUDIOID_XPATH_DATA,     TAG_DISCNUMBER, "/position", NULL },
  /* relative to /metadata/recording/release-list/release/medium */
  { AUDIOID_XPATH_DATA,     TAG_TRACKNUMBER, "/track-list/track/position", NULL },
  { AUDIOID_XPATH_DATA,     TAG_DURATION, "/track-list/track/length", NULL },
  { AUDIOID_XPATH_DATA,     TAG_TRACKTOTAL, "/track-list", "count" },
  { AUDIOID_XPATH_END, -1, NULL, NULL },
};

static void mbWebResponseCallback (void *userdata, const char *resp, size_t len);

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
  mdfree (mb);
}

void
mbRecordingIdLookup (audioidmb_t *mb, const char *recid, ilist_t *respdata)
{
  char          uri [MAXPATHLEN];
//  ilistidx_t    iteridx;
//  ilistidx_t    key;

  /* musicbrainz prefers only one call per second */
  while (! mstimeCheck (&mb->globalreqtimer)) {
    mssleep (10);
  }
  mstimeset (&mb->globalreqtimer, 1000);

  strlcpy (uri, sysvarsGetStr (SV_AUDIOID_MUSICBRAINZ_URI), sizeof (uri));
  strlcat (uri, "/recording/", sizeof (uri));
  strlcat (uri, recid, sizeof (uri));
  /* artist-credits retrieves the additional artists for the song */
  /* media is needed to get the track number and track total */
  /* work-rels is needed to get the work-id */
  /* releases is used to get the list of releases for this recording */
  /*    a match can then possibly be made by album name/track number */
  /* releases is used to get the list of releases for this recording */
  strlcat (uri, "?inc=artist-credits+work-rels+releases+artists+media+isrcs", sizeof (uri));
  logMsg (LOG_DBG, LOG_AUDIO_ID, "audioid: mb: uri: %s", uri);

#if 0
  webclientGet (mb->webclient, uri);
#endif

{
size_t rlen;
mb->webresponse = filedataReadAll ("mb.xml", &rlen);
mb->webresplen = rlen;
}

  if (mb->webresponse != NULL && mb->webresplen > 0) {
    mb->respcount = audioidParseAll (mb->webresponse, mb->webresplen,
        mbxpaths, respdata);
  }

  return;
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

