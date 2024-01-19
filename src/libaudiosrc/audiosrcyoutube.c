/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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
#include "bdjvars.h"
#include "dirlist.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "mdebug.h"
#include "pathinfo.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"

typedef struct asiterdata {
  const char      *uri;
  slist_t         *videolist;
  slistidx_t      viditer;
} asiterdata_t;

bool
audiosrcyoutubeExists (const char *nm)
{
  bool    exists = false;

  return exists;
}

asiterdata_t *
audiosrcyoutubeStartIterator (const char *uri)
{
  asiterdata_t  *asidata;

  if (uri == NULL) {
    return NULL;
  }

  asidata = mdmalloc (sizeof (asiterdata_t));
  asidata->uri = uri;

  // ### get video list from uri
  slistStartIterator (asidata->videolist, &asidata->viditer);
  return asidata;
}

void
audiosrcyoutubeCleanIterator (asiterdata_t *asidata)
{
  if (asidata == NULL) {
    return;
  }

  if (asidata->videolist != NULL) {
    slistFree (asidata->videolist);
  }
  mdfree (asidata);
}

int32_t
audiosrcyoutubeIterCount (asiterdata_t *asidata)
{
  int32_t   c = 0;

  if (asidata == NULL) {
    return c;
  }

  c = slistGetCount (asidata->videolist);
  return c;
}

const char *
audiosrcyoutubeIterate (asiterdata_t *asidata)
{
  const char    *rval = NULL;

  if (asidata == NULL) {
    return NULL;
  }

  rval = slistIterateKey (asidata->videolist, &asidata->viditer);
  return rval;
}

