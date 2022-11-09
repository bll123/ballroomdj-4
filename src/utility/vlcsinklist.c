#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#include "bdjopt.h"
#include "fileop.h"
#include "pli.h"
#include "sysvars.h"
#include "volsink.h"
#include "volume.h"

int
main (int argc, char *argv [])
{
  volsinklist_t sinklist;
  pli_t         *pli;

  if (! fileopIsDirectory ("data")) {
    fprintf (stderr, "run from top level\n");
    exit (1);
  }

  sysvarsInit (argv [0]);
  bdjoptInit ();

  volumeSinklistInit (&sinklist);

  pli = pliInit (bdjoptGetStr (OPT_M_PLAYER_INTFC), "");
  pliAudioDeviceList (pli, &sinklist);

  for (size_t i = 0; i < sinklist.count; ++i) {
    fprintf (stderr, "def: %d idx: %3d\n    nm: %s\n  desc: %s\n",
        sinklist.sinklist [i].defaultFlag,
        sinklist.sinklist [i].idxNumber,
        sinklist.sinklist [i].name,
        sinklist.sinklist [i].description);
  }
  volumeFreeSinkList (&sinklist);

  pliFree (pli);

  bdjoptCleanup ();
  return 0;
}
