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
#include "dirlist.h"
#include "dylib.h"
#include "mdebug.h"
#include "slist.h"
#include "sysvars.h"
#include "volsink.h"
#include "volume.h"

typedef struct volume {
  dlhandle_t  *dlHandle;
  const char  *(*volumeDesc) (void);
  int         (*volumeProcess) (volaction_t, const char *, int *, volsinklist_t *, void **);
  void        (*volumeDisconnect) (void);
  void        (*volumeCleanup) (void **);
  void        *udata;
} volume_t;

volume_t *
volumeInit (const char *volpkg)
{
  volume_t  *volume;
  char      dlpath [MAXPATHLEN];

  volume = mdmalloc (sizeof (volume_t));
  volume->volumeDesc = NULL;
  volume->volumeProcess = NULL;
  volume->volumeDisconnect = NULL;
  volume->volumeCleanup = NULL;
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
  volume->volumeDesc = dylibLookup (volume->dlHandle, "volumeDesc");
  volume->volumeProcess = dylibLookup (volume->dlHandle, "volumeProcess");
  volume->volumeDisconnect = dylibLookup (volume->dlHandle, "volumeDisconnect");
  volume->volumeCleanup = dylibLookup (volume->dlHandle, "volumeCleanup");
#pragma clang diagnostic pop

  return volume;
}

void
volumeFree (volume_t *volume)
{
  if (volume != NULL) {
    volume->volumeCleanup (&volume->udata);
    if (volume->dlHandle != NULL) {
      dylibClose (volume->dlHandle);
    }
    mdfree (volume);
  }
}

const char *
volumeDescription (volume_t *volume)
{
  return volume->volumeDesc ();
}

bool
volumeHaveSinkList (volume_t *volume)
{
  return volume->volumeProcess (VOL_HAVE_SINK_LIST, NULL, NULL, NULL,
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
  volume->volumeProcess (VOL_GET, sinkname, &vol, NULL, &volume->udata);
  volume->volumeDisconnect ();
  return vol;
}

int
volumeSet (volume_t *volume, const char *sinkname, int vol)
{
  volume->volumeProcess (VOL_SET, sinkname, &vol, NULL, &volume->udata);
  volume->volumeDisconnect ();
  return vol;
}

void
volumeSetSystemDefault (volume_t *volume, const char *sinkname)
{
  volume->volumeProcess (VOL_SET_SYSTEM_DFLT, sinkname, NULL, NULL,
      &volume->udata);
  volume->volumeDisconnect ();
}

int
volumeGetSinkList (volume_t *volume, const char *sinkname, volsinklist_t *sinklist)
{
  int     rc;

  rc = volume->volumeProcess (VOL_GETSINKLIST, sinkname, NULL, sinklist,
      &volume->udata);
  volume->volumeDisconnect ();
  return rc;
}

void
volumeFreeSinkList (volsinklist_t *sinklist)
{
  if (sinklist->sinklist != NULL) {
    for (size_t i = 0; i < sinklist->count; ++i) {
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
  slist_t     *files;
  slistidx_t  iteridx;
  const char  *fn;
  dlhandle_t  *dlHandle;
  char        dlpath [MAXPATHLEN];
  char        tmp [100];
  const char  *(*volumeDesc) (void);
  const char  *desc;

  interfaces = slistAlloc ("vol-intfc", LIST_UNORDERED, NULL);
  files = dirlistBasicDirList (sysvarsGetStr (SV_BDJ4_DIR_EXEC), sysvarsGetStr (SV_SHLIB_EXT));
  slistStartIterator (files, &iteridx);
  while ((fn = slistIterateKey (files, &iteridx)) != NULL) {
    if (strncmp (fn, "libvol", 6) == 0) {
      pathbldMakePath (dlpath, sizeof (dlpath), fn, "", PATHBLD_MP_DIR_EXEC);
      dlHandle = dylibLoad (dlpath);
      volumeDesc = dylibLookup (dlHandle, "volumeDesc");
      if (volumeDesc != NULL) {
        desc = volumeDesc ();
        strlcpy (tmp, fn, sizeof (tmp));
        tmp [strlen (tmp) - strlen (sysvarsGetStr (SV_SHLIB_EXT))] = '\0';
        slistSetStr (interfaces, desc, tmp);
      }
      dylibClose (dlHandle);
    }
  }
  slistFree (files);
  slistSort (interfaces);

  return interfaces;
}
