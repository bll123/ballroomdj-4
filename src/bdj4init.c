#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#if _hdr_libintl
# include <libintl.h>
#endif

#include "bdj4init.h"
#include "bdjmsg.h"
#include "musicdb.h"
#include "tagdef.h"
#include "bdjstring.h"
#include "sysvars.h"
#include "bdjvars.h"
#include "bdjvarsdfload.h"
#include "log.h"
#include "tmutil.h"
#include "fileop.h"
#include "portability.h"
#include "pathbld.h"
#include "bdjopt.h"
#include "lock.h"
#include "bdjvars.h"

static void initLocale (void);

static void
initLocale (void)
{
  char    tpath [MAXPATHLEN];

  setlocale (LC_ALL, "");
  pathbldMakePath (tpath, sizeof (tpath), "locale", "", "", PATHBLD_MP_MAINDIR);
  bindtextdomain ("bdj", tpath);
  textdomain ("bdj");
  return;
}

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
  char        *locknm = lockName (route);

  static struct option bdj_options [] = {
    { "main",       no_argument,        NULL,   1 },
    { "playerui",   no_argument,        NULL,   2 },
    { "configui",   no_argument,        NULL,   3 },
    { "manageui",   no_argument,        NULL,   4 },
    { "profile",    required_argument,  NULL,   'p' },
    { "debug",      required_argument,  NULL,   'd' },
    { NULL,         0,                  NULL,   0 }
  };

  mstimestart (&mt);
  initLocale ();
  sRandom ();
  sysvarsInit (argv[0]);

  pathbldMakePath (tbuff, sizeof (tbuff), "", sysvars [SV_HOSTNAME], "", PATHBLD_MP_NONE);
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
          sysvarSetNum (SVL_BDJIDX, atol (optarg));
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
    ssize_t profile = lsysvars [SVL_BDJIDX];
    ++profile;
    sysvarSetNum (SVL_BDJIDX, profile);
    ++count;
    if (count > 20) {
      bdj4shutdown (route);
      exit (1);
    }
    rc = lockAcquire (locknm, PATHBLD_MP_USEIDX);
  }

  if (chdir (sysvars [SV_BDJ4DIR]) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvars [SV_BDJ4DIR]);
    exit (1);
  }
  bdjvarsInit ();

  /* for the time being, leave this as route_main */
  /* this will change in the future */
  /* re-use the lock name as the program name */
  if (route == ROUTE_MAIN) {
    logStart (locknm, tag, loglevel);
  } else {
    logStartAppend (locknm, tag, loglevel);
  }
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  bdjvarsdfloadInit ();
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
