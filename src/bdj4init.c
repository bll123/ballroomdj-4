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
bdj4startup (int argc, char *argv[], char *tag, bdjmsgroute_t route, int flags)
{
  mstime_t    mt;
  mstime_t    dbmt;
  int         c = 0;
  int         rc = 0;
  int         count = 0;
  int         option_index = 0;
  char        tbuff [MAXPATHLEN];
  loglevel_t  loglevel = LOG_IMPORTANT | LOG_MAIN;
  bool        startlog = false;

  static struct option bdj_options [] = {
    { "bdj4main",   no_argument,        NULL,   0 },
    { "main",       no_argument,        NULL,   0 },
    { "bdj4playerui", no_argument,      NULL,   0 },
    { "playerui",   no_argument,        NULL,   0 },
    { "bdj4configui", no_argument,      NULL,   0 },
    { "bdj4manageui", no_argument,      NULL,   0 },
    { "bdj4starterui", no_argument,     NULL,   0 },
    /* bdj4 loader options to ignore */
    { "debugself",  no_argument,        NULL,   0 },
    { "nodetach",   no_argument,        NULL,   0 },
    { "msys",       no_argument,        NULL,   0 },
    /* normal options */
    { "profile",    required_argument,  NULL,   'p' },
    { "debug",      required_argument,  NULL,   'd' },
    { "startlog",   no_argument,        NULL,   's' },
    { NULL,         0,                  NULL,   0 }
  };

  mstimestart (&mt);
  sRandom ();
  sysvarsInit (argv[0]);
  localeInit ();

  pathbldMakePath (tbuff, sizeof (tbuff), "", sysvarsGetStr (SV_HOSTNAME), "", PATHBLD_MP_NONE);
  if (! fileopIsDirectory (tbuff)) {
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
      case 's': {
        startlog = true;
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

  /* re-use the lock name as the program name */
  if (startlog || route == ROUTE_PLAYERUI) {
    logStart (lockName (route), tag, loglevel);
  } else {
    logStartAppend (lockName (route), tag, loglevel);
  }
  logProcBegin (LOG_PROC, "bdj4startup");
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", sysvarsGetNum (SVL_BDJIDX));

  rc = bdjvarsdfloadInit ();
  if (rc < 0) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "Unable to load all data files");
    fprintf (stderr, "Unable to load all data files\n");
    exit (1);
  }

  bdjoptInit ();

  bdjoptSetNum (OPT_G_DEBUGLVL, loglevel);

  if ((flags & BDJ4_INIT_NO_DB_LOAD) != BDJ4_INIT_NO_DB_LOAD) {
    mstimestart (&dbmt);
    logMsg (LOG_SESS, LOG_IMPORTANT, "Database read: started");
    pathbldMakePath (tbuff, MAXPATHLEN, "",
        MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_NONE);
    dbOpen (tbuff);
    logMsg (LOG_SESS, LOG_IMPORTANT, "Database read: %ld items in %ld ms", dbCount(), mstimeend (&dbmt));
  }
  logMsg (LOG_SESS, LOG_IMPORTANT, "Total init time: %ld ms", mstimeend (&mt));

  logProcEnd (LOG_PROC, "bdj4startup", "");
  return 0;
}

void
bdj4shutdown (bdjmsgroute_t route)
{
  mstime_t       mt;

  logProcBegin (LOG_PROC, "bdj4shutdown");
  mstimestart (&mt);
  bdjoptFree ();
  dbClose ();
  bdjvarsdfloadCleanup ();
  bdjvarsCleanup ();
  logMsg (LOG_SESS, LOG_IMPORTANT, "init cleanup time: %ld ms", mstimeend (&mt));
  lockRelease (lockName (route), PATHBLD_MP_USEIDX);
  logProcEnd (LOG_PROC, "bdj4shutdown", "");
}
