/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <memory.h>

#include "volsink.h"
#include "volume.h"

static int gvol [3] = { 30, 20, 10 };
static int gsink = 0;

void
volumeDisconnect (void) {
  return;
}

void
volumeCleanup (void **udata) {
  return;
}

int
volumeProcess (volaction_t action, const char *sinkname,
    int *vol, volsinklist_t *sinklist, void **udata)
{
  if (action == VOL_HAVE_SINK_LIST) {
    return true;
  }

  if (action == VOL_GETSINKLIST) {
    sinklist->defname = strdup ("no-volume");
    sinklist->sinklist = NULL;
    sinklist->count = 3;
    sinklist->sinklist = realloc (sinklist->sinklist,
        sinklist->count * sizeof (volsinkitem_t));
    sinklist->sinklist [0].defaultFlag = 1;
    sinklist->sinklist [0].idxNumber = 0;
    sinklist->sinklist [0].name = strdup ("no-volume");
    sinklist->sinklist [0].description = strdup ("No Volume");
    sinklist->sinklist [1].defaultFlag = 0;
    sinklist->sinklist [1].idxNumber = 1;
    sinklist->sinklist [1].name = strdup ("silence");
    sinklist->sinklist [1].description = strdup ("Silence");
    sinklist->sinklist [2].defaultFlag = 0;
    sinklist->sinklist [2].idxNumber = 2;
    sinklist->sinklist [2].name = strdup ("quiet");
    sinklist->sinklist [2].description = strdup ("Quiet");
    return 0;
  }

  if (action == VOL_SET_SYSTEM_DFLT) {
    if (strcmp (sinkname, "no-volume") == 0) {
      gsink = 0;
    }
    if (strcmp (sinkname, "silence") == 0) {
      gsink = 1;
    }
    if (strcmp (sinkname, "quiet") == 0) {
      gsink = 2;
    }
    return 0;
  }

  if (action == VOL_SET) {
    gvol [gsink] = *vol;
    if (gvol [gsink] < 0) { gvol [gsink] = 0; }
    if (gvol [gsink] > 100) { gvol [gsink] = 100; }
  }

  if (vol != NULL && (action == VOL_SET || action == VOL_GET)) {
    *vol = gvol [gsink];

    return *vol;
  }

  return 0;
}
