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
#include "volume.h"
#include "sysvars.h"

int
main (int argc, char *argv [])
{
  volume_t      *volume;
  volsinklist_t sinklist;
  int           vol;

  sysvarsInit (argv [0]);
  bdjoptInit ();

  volume = volumeInit ();

  if (argc == 2 && strcmp (argv [1], "getsinklist") == 0) {
    volumeGetSinkList (volume, "", &sinklist);

    for (size_t i = 0; i < sinklist.count; ++i) {
      fprintf (stderr, "def: %d idx: %3d\n    nm: %s\n  desc: %s\n",
          sinklist.sinklist [i].defaultFlag,
          sinklist.sinklist [i].idxNumber,
          sinklist.sinklist [i].name,
          sinklist.sinklist [i].description);
    }
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
  bdjoptFree ();
  return 0;
}
