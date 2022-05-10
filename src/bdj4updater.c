/*
 * the update process to
 * update all bdj4 data files to the latest version
 * also handles some settings for new installations.
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
#include "bdjstring.h"
#include "filedata.h"
#include "localeutil.h"
#include "osutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "sysvars.h"

#define UPDATER_TMP_FILE "tmpupdater.txt"

int
main (int argc, char *argv [])
{
  bool    newinstall = false;
  int     c = 0;
  int     option_index = 0;
  char    tbuff [MAXPATHLEN];
  bool    isbdj4 = false;

  static struct option bdj_options [] = {
    { "newinstall", no_argument,        NULL,   'n' },
    { "bdj4updater",no_argument,        NULL,   0 },
    { "bdj4",       no_argument,        NULL,   'B' },
    /* ignored */
    { "nodetach",     no_argument,      NULL,   0 },
    { "debugself",    no_argument,      NULL,   0 },
    { "msys",         no_argument,      NULL,   0 },
    { "theme",        required_argument,NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

  while ((c = getopt_long_only (argc, argv, "g:r:u:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      case 'n': {
        newinstall = true;
        break;
      }
      default: {
        break;
      }
    }
  }

  if (! isbdj4) {
    fprintf (stderr, "not started with launcher\n");
    exit (1);
  }

  sysvarsInit (argv [0]);
  localeInit ();
  bdjoptInit ();

  if (newinstall) {
    char  *tval;

    tval = bdjoptGetStr (OPT_M_VOLUME_INTFC);
    if (tval != NULL &&
        strcmp (tval, "libvolnull") == 0) {
      if (isWindows ()) {
        bdjoptSetStr (OPT_M_VOLUME_INTFC, "libvolwin");
      }
      if (isMacOS ()) {
        bdjoptSetStr (OPT_M_VOLUME_INTFC, "libvolmac");
      }
      if (isLinux ()) {
        bdjoptSetStr (OPT_M_VOLUME_INTFC, "libvolpa");
      }
    }

    tval = bdjoptGetStr (OPT_M_STARTUPSCRIPT);
    if (isLinux () &&
        tval != NULL &&
        strcmp (tval, "") == 0) {
      pathbldMakePath (tbuff, sizeof (tbuff),
        "scripts/linux/bdjstartup", ".sh", PATHBLD_MP_MAINDIR);
      bdjoptSetStr (OPT_M_STARTUPSCRIPT, tbuff);
      pathbldMakePath (tbuff, sizeof (tbuff),
        "scripts/linux/bdjshutdown", ".sh", PATHBLD_MP_MAINDIR);
      bdjoptSetStr (OPT_M_SHUTDOWNSCRIPT, tbuff);
    }

    tval = bdjoptGetStr (OPT_M_DIR_MUSIC);
    if (tval != NULL && strcmp (tval, "") == 0) {
      char  *home;

      home = sysvarsGetStr (SV_HOME);

      if (isLinux ()) {
        char  *data = NULL;
        char  *prog = NULL;
        char  *arg = "MUSIC";

        prog = sysvarsGetStr (SV_PATH_XDGUSERDIR);
        if (*prog) {
          data = filedataGetProgOutput (prog, arg, UPDATER_TMP_FILE);
          stringTrim (data);
        }

        snprintf (tbuff, sizeof (tbuff), "%s/", home);
        if (data != NULL &&
            strcmp (data, home) != 0 &&
            strcmp (data, tbuff) != 0) {
          strlcpy (tbuff, data, sizeof (tbuff));
        } else {
          snprintf (tbuff, sizeof (tbuff), "%s/Music", home);
        }
        if (data != NULL) {
          free (data);
        }
      }
      if (isWindows ()) {
        char    *data;

        snprintf (tbuff, sizeof (tbuff), "%s/Music", home);
        data = osRegistryGet (
            "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
            "My Music");
        if (data != NULL && *data) {
          /* windows returns the path with %USERPROFILE% */
          strlcpy (tbuff, home, sizeof (tbuff));
          strlcat (tbuff, data + 13, sizeof (tbuff));
        } else {
          data = osRegistryGet (
              "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
              "My Music");
          if (data != NULL && *data) {
            /* windows returns the path with %USERPROFILE% */
            strlcpy (tbuff, home, sizeof (tbuff));
            strlcat (tbuff, data + 13, sizeof (tbuff));
          }
        }
      }
      if (isMacOS ()) {
        snprintf (tbuff, sizeof (tbuff), "%s/Music", home);
      }
      pathNormPath (tbuff, sizeof (tbuff));
      bdjoptSetStr (OPT_M_DIR_MUSIC, tbuff);
    }

    bdjoptSave ();
  }

  return 0;
}
