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

typedef struct audioidacoustid {
  webclient_t   *webclient;
  const char    *webresponse;
  size_t        webresplen;
} audioidacoustid_t;

static void acoustidWebResponseCallback (void *userdata, const char *resp, size_t len);

audioidacoustid_t *
acoustidInit (void)
{
  audioidacoustid_t *acoustid;

  acoustid = mdmalloc (sizeof (audioidacoustid_t));
  acoustid->webclient = webclientAlloc (acoustid, acoustidWebResponseCallback);
  acoustid->webresponse = NULL;
  acoustid->webresplen = 0;

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

nlist_t *
acoustidRecordingIdLookup (audioidacoustid_t *acoustid, const char *recid)
{
  nlist_t *resp = NULL;


  return resp;
}

static void
acoustidWebResponseCallback (void *userdata, const char *resp, size_t len)
{
  audioidacoustid_t   *acoustid = userdata;

  acoustid->webresponse = resp;
  acoustid->webresplen = len;
  return;
}
