/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#if _hdr_MacTypes
# if defined (BDJ4_MEM_DEBUG)
#  undef BDJ4_MEM_DEBUG
# endif
#endif

#include "bdj4.h"
#include "bdjstring.h"
#include "pathbld.h"
#include "dyintfc.h"
#include "dylib.h"
#include "mdebug.h"
#include "slist.h"
#include "sysvars.h"
#include "volsink.h"
#include "volume.h"

typedef struct volume {
  dlhandle_t  *dlHandle;
  int         (*voliProcess) (volaction_t, const char *, int *, volsinklist_t *, void **);
  void        (*voliDisconnect) (void);
  void        (*voliCleanup) (void **);
  void        *udata;
} volume_t;

volume_t *
volumeInit (const char *volpkg)
{
  volume_t  *volume;
  char      dlpath [MAXPATHLEN];

  volume = mdmalloc (sizeof (volume_t));
  volume->voliProcess = NULL;
  volume->voliDisconnect = NULL;
  volume->voliCleanup = NULL;
  volume->udata = NULL;

  pathbldMakePath (dlpath, sizeof (dlpath),
      volpkg, sysvarsGetStr (SV_SHLIB_EXT), PATHBLD_MP_DIR_EXEC);
  volume->dlHandle = dylibLoad (dlpath);
  if (volume->dlHandle == NULL) {
    fprintf (stderr, "Unable to open library %s\n", dlpath);
    volumeFree (volume);
    return NULL;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
  volume->voliProcess = dylibLookup (volume->dlHandle, "voliProcess");
  volume->voliDisconnect = dylibLookup (volume->dlHandle, "voliDisconnect");
  volume->voliCleanup = dylibLookup (volume->dlHandle, "voliCleanup");
#pragma clang diagnostic pop

  return volume;
}

void
volumeFree (volume_t *volume)
{
  if (volume != NULL) {
    volume->voliCleanup (&volume->udata);
    if (volume->dlHandle != NULL) {
      dylibClose (volume->dlHandle);
    }
    mdfree (volume);
  }
}

bool
volumeHaveSinkList (volume_t *volume)
{
  return volume->voliProcess (VOL_HAVE_SINK_LIST, NULL, NULL, NULL,
      &volume->udata);
}

void
volumeSinklistInit (volsinklist_t *sinklist)
{
  sinklist->defname = NULL;
  sinklist->count = 0;
  sinklist->sinklist = NULL;
}

int
volumeGet (volume_t *volume, const char *sinkname)
{
  int               vol;

  vol = 0;
  volume->voliProcess (VOL_GET, sinkname, &vol, NULL, &volume->udata);
  volume->voliDisconnect ();
  return vol;
}

int
volumeSet (volume_t *volume, const char *sinkname, int vol)
{
  volume->voliProcess (VOL_SET, sinkname, &vol, NULL, &volume->udata);
  volume->voliDisconnect ();
  return vol;
}

void
volumeSetSystemDefault (volume_t *volume, const char *sinkname)
{
  volume->voliProcess (VOL_SET_SYSTEM_DFLT, sinkname, NULL, NULL,
      &volume->udata);
  volume->voliDisconnect ();
}

int
volumeGetSinkList (volume_t *volume, const char *sinkname, volsinklist_t *sinklist)
{
  int     rc;

  rc = volume->voliProcess (VOL_GETSINKLIST, sinkname, NULL, sinklist,
      &volume->udata);
  volume->voliDisconnect ();
  return rc;
}

void
volumeFreeSinkList (volsinklist_t *sinklist)
{
  if (sinklist->sinklist != NULL) {
    for (int i = 0; i < sinklist->count; ++i) {
      dataFree (sinklist->sinklist [i].name);
      dataFree (sinklist->sinklist [i].description);
    }
    dataFree (sinklist->defname);
    mdfree (sinklist->sinklist);
    sinklist->defname = NULL;
    sinklist->count = 0;
    sinklist->sinklist = NULL;
  }
}

slist_t *
volumeInterfaceList (void)
{
  slist_t     *interfaces;

  interfaces = dyInterfaceList ("libvol", "voliDesc");
  return interfaces;
}

char *
volumeCheckInterface (const char *volintfc)
{
  slist_t     *interfaces;
  slistidx_t  iteridx;
  const char  *desc;
  const char  *intfc;
  const char  *gintfc = NULL;
  bool        found = false;
  char        *voli = NULL;

  interfaces = volumeInterfaceList ();
  slistStartIterator (interfaces, &iteridx);
  while ((desc = slistIterateKey (interfaces, &iteridx)) != NULL) {
    intfc = slistGetStr (interfaces, desc);
    if (gintfc == NULL && strcmp (intfc, "libvolnull") != 0) {
      gintfc = intfc;
    }
    if (strcmp (volintfc, intfc) == 0) {
      found = true;
      break;
    }
  }

  if (! found) {
    voli = mdstrdup (gintfc);
  } else {
    voli = mdstrdup (volintfc);
  }
  slistFree (interfaces);

  return voli;
}

