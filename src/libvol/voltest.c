/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
#include "volsink.h"
#include "volume.h"
#include "sysvars.h"

int
main (int argc, char *argv [])
{
  volume_t      *volume;
  volsinklist_t sinklist;
  int           vol;

  if (argc < 2) {
    fprintf (stdout, "usage: voltest {getsinklist|setdflt <sink>|get <sink>|set <sink> <vol>}\n");
    exit (1);
  }

  if (! fileopIsDirectory ("data")) {
    fprintf (stderr, "run from top level\n");
    exit (1);
  }

  sysvarsInit (argv [0]);
  bdjoptInit ();

  volume = volumeInit (bdjoptGetStr (OPT_M_VOLUME_INTFC));
  volumeSinklistInit (&sinklist);

  if (argc == 2 && strcmp (argv [1], "getsinklist") == 0) {
    volumeGetSinkList (volume, "", &sinklist);

    for (size_t i = 0; i < sinklist.count; ++i) {
      fprintf (stderr, "def: %d idx: %3d\n    nm: %s\n  desc: %s\n",
          sinklist.sinklist [i].defaultFlag,
          sinklist.sinklist [i].idxNumber,
          sinklist.sinklist [i].name,
          sinklist.sinklist [i].description);
    }
    volumeFreeSinkList (&sinklist);
  }
  if (argc == 3 && strcmp (argv [1], "setdflt") == 0) {
    volumeSetSystemDefault (volume, argv [2]);
  }
  if (argc == 3 && strcmp (argv [1], "get") == 0) {
    vol = volumeGet (volume, argv [2]);
    fprintf (stderr, "vol: %d\n", vol);
  }
  if (argc == 4 && strcmp (argv [1], "set") == 0) {
    vol = volumeSet (volume, argv [2], atoi (argv [3]));
    fprintf (stderr, "vol: %d\n", vol);
  }

  volumeFree (volume);
  bdjoptCleanup ();
  return 0;
}
