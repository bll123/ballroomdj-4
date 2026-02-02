/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "dyintfc.h"
#include "fileop.h"
#include "mdebug.h"
#include "volsink.h"
#include "volume.h"
#include "ilist.h"
#include "sysvars.h"

int
main (int argc, char *argv [])
{
  volume_t      *volume = NULL;
  volsinklist_t sinklist;
  int           vol;
  bool          doinit = true;

  if (argc < 2) {
    fprintf (stdout, "usage: voltest {getsinklist|get <sink>|set <sink> <vol>}\n");
    exit (1);
  }

  if (! fileopIsDirectory ("data")) {
    fprintf (stderr, "run from top level\n");
    exit (1);
  }

  mdebugInit ("tvol");
  sysvarsInit (argv [0], SYSVARS_FLAG_ALL);
  bdjoptInit ();

  if (argc == 2 && strcmp (argv [1], "interfaces") == 0) {
    ilist_t     *interfaces;
    ilistidx_t  iteridx;
    ilistidx_t  key;

    interfaces = volumeInterfaceList ();
    ilistStartIterator (interfaces, &iteridx);
    while ((key = ilistIterateKey (interfaces, &iteridx)) >= 0) {
      fprintf (stderr, "%s %s\n",
          ilistGetStr (interfaces, key, DYI_DESC),
          ilistGetStr (interfaces, key, DYI_LIB));
    }
    ilistFree (interfaces);
    doinit = false;
  }

  if (doinit) {
    volume = volumeInit (bdjoptGetStr (OPT_M_VOLUME_INTFC));
    volumeInitSinkList (&sinklist);
  }

  if (argc == 2 && strcmp (argv [1], "getsinklist") == 0) {
    volumeGetSinkList (volume, "", &sinklist);

    for (int i = 0; i < sinklist.count; ++i) {
      fprintf (stderr, "def: %d idx: %3d\n    nm: %s\n  desc: %s\n",
          sinklist.sinklist [i].defaultFlag,
          sinklist.sinklist [i].idxNumber,
          sinklist.sinklist [i].name,
          sinklist.sinklist [i].description);
    }
    volumeFreeSinkList (&sinklist);
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
  mdebugReport ();
  mdebugCleanup ();
  return 0;
}
