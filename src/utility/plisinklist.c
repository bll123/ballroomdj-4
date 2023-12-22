/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#include "bdjopt.h"
#include "fileop.h"
#include "mdebug.h"
#include "pli.h"
#include "sysvars.h"
#include "volsink.h"
#include "volume.h"

int
main (int argc, char *argv [])
{
  volsinklist_t sinklist;
  pli_t         *pli;

#if BDJ4_MEM_DEBUG
  mdebugInit ("tvlc");
#endif

  if (! fileopIsDirectory ("data")) {
    fprintf (stderr, "run from top level\n");
    exit (1);
  }

  sysvarsInit (argv [0]);
  bdjoptInit ();

  volumeSinklistInit (&sinklist);

  pli = pliInit (bdjoptGetStr (OPT_M_PLAYER_INTFC));
  pliAudioDeviceList (pli, &sinklist);

  for (int i = 0; i < sinklist.count; ++i) {
    fprintf (stdout, "def: %d idx: %3d\n    nm: %s\n  desc: %s\n",
        sinklist.sinklist [i].defaultFlag,
        sinklist.sinklist [i].idxNumber,
        sinklist.sinklist [i].name,
        sinklist.sinklist [i].description);
  }
  volumeFreeSinkList (&sinklist);

  pliFree (pli);

  bdjoptCleanup ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}
