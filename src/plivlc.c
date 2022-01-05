#include "config.h"
#include "pconfig.h"

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
  vlcMedia (pliData->plData, afname);
}

void
pliStartPlayback (plidata_t *pliData)
{
  vlcPlay (pliData->plData);
}

void
pliPause (plidata_t *pliData)
{
  vlcPause (pliData->plData);
}

void
pliPlay (plidata_t *pliData)
{
  vlcPlay (pliData->plData);
}

void
pliStop (plidata_t *pliData)
{
  vlcStop (pliData->plData);
}

void
pliClose (plidata_t *pliData)
{
  vlcClose (pliData->plData);
}

#endif /* have libvlc_new */
