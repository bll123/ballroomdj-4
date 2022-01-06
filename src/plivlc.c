#include "config.h"
#include "configp.h"

#if _lib_libvlc_new

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "pli.h"
#include "vlci.h"

static char *   vlcDefaultOptions [] = {
      "--quiet",
      "--intf=dummy",
      "--audio-filter", "scaletempo",
      "--ignore-config",
      "--no-media-library",
      "--no-playlist-autostart",
      "--no-random",
      "--no-loop",
      "--no-repeat",
      "--play-and-stop",
      "--novideo",
};
#define VLC_DFLT_OPT_SZ (sizeof (vlcDefaultOptions) / sizeof (char *))

plidata_t *
pliInit (void)
{
  plidata_t     *pliData;

  pliData = malloc (sizeof (plidata_t));
  assert (pliData != NULL);
  pliData->plData = vlcInit (VLC_DFLT_OPT_SZ, vlcDefaultOptions);
  assert (pliData->plData != NULL);
  return pliData;
}

void
pliFree (plidata_t *pliData)
{
  if (pliData != NULL) {
    pliClose (pliData);
    free (pliData);
  }
}

void
pliMediaSetup (plidata_t *pliData, char *afname)
{
  if (pliData != NULL && pliData->plData != NULL) {
    vlcMedia (pliData->plData, afname);
  }
}

void
pliStartPlayback (plidata_t *pliData)
{
  if (pliData != NULL && pliData->plData != NULL) {
    vlcPlay (pliData->plData);
  }
}

void
pliPause (plidata_t *pliData)
{
  if (pliData != NULL && pliData->plData != NULL) {
    vlcPause (pliData->plData);
  }
}

void
pliPlay (plidata_t *pliData)
{
  if (pliData != NULL && pliData->plData != NULL) {
    vlcPlay (pliData->plData);
  }
}

void
pliStop (plidata_t *pliData)
{
  if (pliData != NULL && pliData->plData != NULL) {
    vlcStop (pliData->plData);
  }
}

void
pliClose (plidata_t *pliData)
{
  if (pliData != NULL) {
    if (pliData->plData != NULL) {
      vlcClose (pliData->plData);
    }
    pliData->plData = NULL;
  }
}

#endif /* have libvlc_new */
