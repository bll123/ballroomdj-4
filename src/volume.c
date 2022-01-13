#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "bdjopt.h"
#include "datautil.h"
#include "dylib.h"
#include "portability.h"
#include "sysvars.h"
#include "volume.h"

volume_t *
volumeInit (void)
{
  volume_t      *volume;

  char      dlpath [MAXPATHLEN];

  volume = malloc (sizeof (volume_t));
  assert (volume != NULL);
  volume->volumeProcess = NULL;
  volume->volumeDisconnect = NULL;

  datautilMakePath (dlpath, sizeof (dlpath), "",
      bdjoptGetData (OPT_G_VOLUME_INTFC),
      sysvars [SV_SHLIB_EXT], DATAUTIL_MP_EXECDIR);
  volume->dlHandle = dylibLoad (dlpath);
  if (volume->dlHandle == NULL) {
    volumeFree (volume);
    return NULL;
  }

  volume->volumeProcess = dylibLookup (volume->dlHandle, "volumeProcess");
  volume->volumeDisconnect = dylibLookup (volume->dlHandle, "volumeDisconnect");

  return volume;
}

void
volumeFree (volume_t *volume)
{
  if (volume != NULL) {
    if (volume->dlHandle != NULL) {
      dylibClose (volume->dlHandle);
    }
    free (volume);
  }
}


int
volumeGet (volume_t *volume, char *sinkname)
{
  int               vol;
  int               rc;

  vol = 0;
  rc = volume->volumeProcess (VOL_GET, sinkname, &vol, NULL);
  volume->volumeDisconnect ();
  return vol;
}

int
volumeSet (volume_t *volume, char *sinkname, int vol)
{
  int               rc;

  rc = volume->volumeProcess (VOL_SET, sinkname, &vol, NULL);
  volume->volumeDisconnect ();
  return vol;
}


int
volumeGetSinkList (volume_t *volume, char *sinkname, volsinklist_t *sinklist)
{
  int               rc;
  int               vol;

  vol = 0;
  rc = volume->volumeProcess (VOL_GETSINKLIST, sinkname, &vol, sinklist);
  volume->volumeDisconnect ();
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
