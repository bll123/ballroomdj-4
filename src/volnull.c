#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <memory.h>

#include "volume.h"

static int gvol = 30;

void
volumeDisconnect (void) {
  return;
}

int
volumeProcess (volaction_t action, char *sinkname,
    int *vol, volsinklist_t *sinklist)
{
  if (action == VOL_GETSINKLIST) {
    sinklist->defname = "default-null";
    sinklist->sinklist = NULL;
    sinklist->count = 1;
    sinklist->sinklist = realloc (sinklist->sinklist,
        sinklist->count * sizeof (volsinkitem_t));
    sinklist->sinklist [0].defaultFlag = 1;
    sinklist->sinklist [0].idxNumber = 1;
    sinklist->sinklist [0].name = strdup ("default-null");
    sinklist->sinklist [0].description = strdup ("Null Default");
  }
  if (action == VOL_SET) {
    gvol = *vol;
  }

  *vol = gvol;
  return 0;
}
