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
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"

typedef struct audioid {
  audioidmb_t     *mb;
} audioid_t;

audioid_t *
audioidInit (void)
{
  audioid_t *audioid;

  audioid = mdmalloc (sizeof (audioid_t));
  audioid->mb = mbInit ();
  return audioid;
}

void
audioidFree (audioid_t *audioid)
{
  if (audioid == NULL) {
    return;
  }

  mbFree (audioid->mb);
  mdfree (audioid);
}

nlist_t *
audioidLookup (audioid_t *audioid, const song_t *song)
{
  const char  *mbrecid;
  nlist_t     *resp = NULL;

  mbrecid = songGetStr (song, TAG_RECORDING_ID);
  if (mbrecid != NULL) {
    mbRecordingIdLookup (audioid->mb, mbrecid);
  }

  return resp;
}
