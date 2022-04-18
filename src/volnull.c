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
    sinklist->defname = "no-volume";
    sinklist->sinklist = NULL;
    sinklist->count = 3;
    sinklist->sinklist = realloc (sinklist->sinklist,
        sinklist->count * sizeof (volsinkitem_t));
    sinklist->sinklist [0].defaultFlag = 1;
    sinklist->sinklist [0].idxNumber = 1;
    sinklist->sinklist [0].name = strdup ("no-volume");
    sinklist->sinklist [0].description = strdup ("No Volume");
    sinklist->sinklist [1].defaultFlag = 0;
    sinklist->sinklist [1].idxNumber = 2;
    sinklist->sinklist [1].name = strdup ("silence");
    sinklist->sinklist [1].description = strdup ("Silence");
    sinklist->sinklist [2].defaultFlag = 0;
    sinklist->sinklist [2].idxNumber = 3;
    sinklist->sinklist [2].name = strdup ("quiet");
    sinklist->sinklist [2].description = strdup ("Quiet");
  }
  if (action == VOL_SET) {
    gvol = *vol;
  }

  *vol = gvol;
  return 0;
}
