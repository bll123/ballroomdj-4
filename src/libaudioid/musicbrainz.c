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

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "audioid.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "sysvars.h"
#include "webclient.h"

typedef struct audioidmb {
  webclient_t   *webclient;
  const char    *webresponse;
  size_t        webresplen;
} audioidmb_t;

static void mbWebResponseCallback (void *userdata, const char *resp, size_t len);

audioidmb_t *
mbInit (void)
{
  audioidmb_t *mb;

  mb = mdmalloc (sizeof (audioidmb_t));
  mb->webclient = webclientAlloc (mb, mbWebResponseCallback);
  mb->webresponse = NULL;
  mb->webresplen = 0;

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

nlist_t *
mbRecordingIdLookup (audioidmb_t *mb, const char *recid)
{
  char    uri [MAXPATHLEN];
  nlist_t *resp = NULL;

  strlcpy (uri, sysvarsGetStr (SV_AUDIOID_MUSICBRAINZ_URI), sizeof (uri));
  strlcat (uri, recid, sizeof (uri));
  /* artist-credits retrieves the additional artists for the song */
  /* media is needed to get the track number and track total */
  /* work-rels is needed to get the work-id */
  /* releases is used to get the list of releases for this recording */
  /*    a match can then possibly be made by album name/track number */
  /* releases is used to get the list of releases for this recording */
  strlcat (uri, "?inc=artist-credits+work-rels+releases+artists+media", sizeof (uri));

  webclientGet (mb->webclient, uri);
  if (mb->webresponse != NULL) {
fprintf (stderr, "resp:\n%.*s", (int) mb->webresplen, mb->webresponse);
  }

  return resp;
}

static void
mbWebResponseCallback (void *userdata, const char *resp, size_t len)
{
  audioidmb_t   *mb = userdata;

  mb->webresponse = resp;
  mb->webresplen = len;
  return;
}
