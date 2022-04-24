/*
 * runs the upgrade process to
 * update all bdj4 data files to the latest version
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
#include <getopt.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "localeutil.h"
#include "pathbld.h"
#include "sysvars.h"

int
main (int argc, char *argv [])
{
  bool          newinstall = false;
  int           c = 0;
  int           option_index = 0;
  char          tbuff [MAXPATHLEN];

  static struct option bdj_options [] = {
    { "newinstall", no_argument,        NULL,   'n' },
    { "bdj4updater",no_argument,        NULL,   0 },
    { "bdj4",       no_argument,        NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

  while ((c = getopt_long_only (argc, argv, "g:r:u:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'n': {
        newinstall = true;
        break;
      }
      default: {
        break;
      }
    }
  }

  sysvarsInit (argv [0]);
  localeInit ();
  bdjoptInit ();

  if (newinstall &&
      strcmp (bdjoptGetStr (OPT_M_VOLUME_INTFC), "libvolnull") == 0) {
    if (isWindows ()) {
      bdjoptSetStr (OPT_M_VOLUME_INTFC, "libvolwin");
    }
    if (isMacOS ()) {
      bdjoptSetStr (OPT_M_VOLUME_INTFC, "libvolmv");
    }
    if (isLinux ()) {
      bdjoptSetStr (OPT_M_VOLUME_INTFC, "libvolpa");
    }
  }

  if (newinstall &&
      isLinux () &&
      strcmp (bdjoptGetStr (OPT_M_STARTUPSCRIPT), "") == 0) {
    pathbldMakePath (tbuff, sizeof (tbuff),
      "scripts/linux/bdjstartup", ".sh", PATHBLD_MP_MAINDIR);
    bdjoptSetStr (OPT_M_STARTUPSCRIPT, tbuff);
    pathbldMakePath (tbuff, sizeof (tbuff),
      "scripts/linux/bdjshutdown", ".sh", PATHBLD_MP_MAINDIR);
    bdjoptSetStr (OPT_M_SHUTDOWNSCRIPT, tbuff);
  }

  if (newinstall) {
    bdjoptSave ();
  }

  return 0;
}
