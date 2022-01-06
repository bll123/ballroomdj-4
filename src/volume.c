/*
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "volume.h"

int
volumeGet (char *sinkname)
{
  int               vol;
  int               rc;

  vol = 0;
  rc = volumeProcess (VOL_GET, sinkname, &vol, NULL);
  volumeDisconnect ();
  return vol;
}

int
volumeSet (char *sinkname, int vol)
{
  int               rc;

  rc = volumeProcess (VOL_SET, sinkname, &vol, NULL);
  volumeDisconnect ();
  return vol;
}


int
volumeGetSinkList (char *sinkname, volsinklist_t *sinklist)
{
  int               rc;
  int               vol;

  vol = 0;
  rc = volumeProcess (VOL_GETSINKLIST, sinkname, &vol, sinklist);
  volumeDisconnect ();
  return vol;
}

void
volumeFreeSinkList (volsinklist_t *sinklist)
{
  if (sinklist != NULL) {
    if (sinklist->sinklist != NULL) {
      for (size_t i = 0; i < sinklist->count; ++i) {
        if (sinklist->sinklist [i].name != NULL) {
          free (sinklist->sinklist [i].name);
        }
        if (sinklist->sinklist [i].description != NULL) {
          free (sinklist->sinklist [i].description);
        }
      }
      free (sinklist->sinklist);
      sinklist->defname = "";
      sinklist->count = 0;
      sinklist->sinklist = NULL;
    }
  }
}

