#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "pathbld.h"
#include "dylib.h"
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

  pathbldMakePath (dlpath, sizeof (dlpath), "",
      bdjoptGetStr (OPT_M_VOLUME_INTFC),
      sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_EXECDIR);
  volume->dlHandle = dylibLoad (dlpath);
  if (volume->dlHandle == NULL) {
    fprintf (stderr, "Unable to open library %s\n", dlpath);
    volumeFree (volume);
    return NULL;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
  volume->volumeProcess = dylibLookup (volume->dlHandle, "volumeProcess");
  assert (volume->volumeProcess != NULL);
  volume->volumeDisconnect = dylibLookup (volume->dlHandle, "volumeDisconnect");
  assert (volume->volumeDisconnect != NULL);
#pragma clang diagnostic pop

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

  vol = 0;
  volume->volumeProcess (VOL_GET, sinkname, &vol, NULL);
  volume->volumeDisconnect ();
  return vol;
}

int
volumeSet (volume_t *volume, char *sinkname, int vol)
{
  volume->volumeProcess (VOL_SET, sinkname, &vol, NULL);
  volume->volumeDisconnect ();
  return vol;
}


int
volumeGetSinkList (volume_t *volume, char *sinkname, volsinklist_t *sinklist)
{
  int               vol;

  vol = 0;
  volume->volumeProcess (VOL_GETSINKLIST, sinkname, &vol, sinklist);
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
