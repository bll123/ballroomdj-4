/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdj4arg.h"
#include "bdjstring.h"
#include "fileop.h"
#include "dirop.h"
#include "mdebug.h"
#include "osdirutil.h"
#include "pathbld.h"
#include "sysvars.h"
#include "tmutil.h"

static void cleantmpClean (void);

int
main (int argc, char *argv [])
{
  bdj4arg_t   *bdj4arg;
  const char  *targ;
  char        tbuff [MAXPATHLEN];

  bdj4arg = bdj4argInit (argc, argv);

  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ);

  /* bdj4cleantmp is started by bdj4, and will be in the datatopdir */

  /* clean up the volreg.txt file */
  pathbldMakePath (tbuff, sizeof (tbuff),
      VOLREG_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_CACHE);
  fileopDelete (tbuff);

  cleantmpClean ();

  for (int i = 1; i < 10; ++i) {
    char  tfn [MAXPATHLEN];

    snprintf (tfn, sizeof (tfn), "%s/%s%s%02d%s",
        sysvarsGetStr (SV_DIR_CONFIG), ALT_INST_PATH_FN,
        sysvarsGetStr (SV_BDJ4_DEVELOPMENT), i, BDJ4_CONFIG_EXT);
    if (fileopFileExists (tfn)) {
      char  altdir [MAXPATHLEN];
      FILE  *fh;

      fh = fileopOpen (tfn, "r");
      *altdir = '\0';
      if (fh != NULL) {
        (void) ! fgets (altdir, sizeof (altdir), fh);
        stringTrim (altdir);
        mdextfclose (fh);
        fclose (fh);
      }

      if (*altdir) {
        if (osChangeDir (altdir) == 0) {
          cleantmpClean ();
        }
      }
    }
  }

  bdj4argCleanup (bdj4arg);
  return 0;
}

static void
cleantmpClean (void)
{
  const char  *fn;
  int         count;

  fn = "tmp";

  count = 0;
  while (fileopIsDirectory (fn)) {
    diropDeleteDir (fn, DIROP_ALL);
    mssleep (100);
    ++count;
    if (count > 5) {
      return;
    }
  }

  diropMakeDir (fn);
}
