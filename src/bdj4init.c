#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdfload.h"
#include "bdjvars.h"
#include "bdjvars.h"
#include "fileop.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "musicdb.h"
#include "pathbld.h"
#include "osutils.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"

int
bdj4startup (int argc, char *argv[], char *tag, bdjmsgroute_t route)
{
  mstime_t    mt;
  mstime_t    dbmt;
  int         c = 0;
  int         rc = 0;
  int         count = 0;
  int         option_index = 0;
  char        tbuff [MAXPATHLEN];
  loglevel_t  loglevel = LOG_IMPORTANT | LOG_MAIN;

  static struct option bdj_options [] = {
    { "bdj4main",   no_argument,        NULL,   1 },
    { "main",       no_argument,        NULL,   1 },
    { "bdj4playerui", no_argument,      NULL,   2 },
    { "playerui",   no_argument,        NULL,   2 },
    { "bdj4configui", no_argument,      NULL,   3 },
    { "bdj4manageui", no_argument,      NULL,   4 },
    { "profile",    required_argument,  NULL,   'p' },
    { "debug",      required_argument,  NULL,   'd' },
    { NULL,         0,                  NULL,   0 }
  };

  mstimestart (&mt);
  sRandom ();
  sysvarsInit (argv[0]);
  localeInit ();

  pathbldMakePath (tbuff, sizeof (tbuff), "", sysvarsGetStr (SV_HOSTNAME), "", PATHBLD_MP_NONE);
  if (! fileopExists (tbuff)) {
    fileopMakeDir (tbuff);
  }

  while ((c = getopt_long_only (argc, argv, "p:d:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        if (optarg) {
          loglevel = (loglevel_t) atoi (optarg);
        }
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarsSetNum (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  rc = lockAcquire (lockName (route), PATHBLD_MP_USEIDX);
  count = 0;
  while (rc < 0) {
      /* try for the next free profile */
    ssize_t profile = sysvarsGetNum (SVL_BDJIDX);
    ++profile;
    sysvarsSetNum (SVL_BDJIDX, profile);
    ++count;
    if (count > 20) {
      bdj4shutdown (route);
      exit (1);
    }
    rc = lockAcquire (lockName (route), PATHBLD_MP_USEIDX);
  }

  if (chdir (sysvarsGetStr (SV_BDJ4DATATOPDIR)) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvarsGetStr (SV_BDJ4DATATOPDIR));
    exit (1);
  }
  bdjvarsInit ();

  /* for the time being, leave this as route_main */
  /* this will change in the future */
  /* re-use the lock name as the program name */
  if (route == ROUTE_MAIN) {
    logStart (lockName (route), tag, loglevel);
  } else {
    logStartAppend (lockName (route), tag, loglevel);
  }
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", sysvarsGetNum (SVL_BDJIDX));

  rc = bdjvarsdfloadInit ();
  if (rc < 0) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "Unable to load all data files");
    fprintf (stderr, "Unable to load all data files\n");
    exit (1);
  }

  bdjoptInit ();

  bdjoptSetNum (OPT_G_DEBUGLVL, loglevel);

  mstimestart (&dbmt);
  pathbldMakePath (tbuff, MAXPATHLEN, "",
      MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_NONE);
  dbOpen (tbuff);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Database read: %ld items in %ld ms", dbCount(), mstimeend (&dbmt));
  logMsg (LOG_SESS, LOG_IMPORTANT, "Total init time: %ld ms", mstimeend (&mt));

  return 0;
}

void
bdj4shutdown (bdjmsgroute_t route)
{
  mstime_t       mt;

  mstimestart (&mt);
  bdjoptFree ();
  dbClose ();
  bdjvarsdfloadCleanup ();
  bdjvarsCleanup ();
  logMsg (LOG_SESS, LOG_IMPORTANT, "init cleanup time: %ld ms", mstimeend (&mt));
  lockRelease (lockName (route), PATHBLD_MP_USEIDX);
}
