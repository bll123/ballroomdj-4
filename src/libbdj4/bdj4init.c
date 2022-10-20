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

#include "audiotag.h"
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
#include "osrandom.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"

int
bdj4startup (int argc, char *argv[], musicdb_t **musicdb,
    char *tag, bdjmsgroute_t route, int flags)
{
  mstime_t    mt;
  mstime_t    dbmt;
  int         c = 0;
  int         rc = 0;
  int         count = 0;
  int         option_index = 0;
  char        tbuff [MAXPATHLEN];
  loglevel_t  loglevel = LOG_IMPORTANT | LOG_MAIN;
  bool        loglevelset = false;
  bool        startlog = false;
  bool        isbdj4 = false;

  static struct option bdj_options [] = {
    { "bdj4bpmcounter",no_argument,     NULL,   0 },
    { "bdj4configui", no_argument,      NULL,   0 },
    { "bdj4dbupdate", no_argument,      NULL,   0 },
    { "bdj4helperui", no_argument,      NULL,   0 },
    { "bdj4main",     no_argument,      NULL,   0 },
    { "bdj4manageui", no_argument,      NULL,   0 },
    { "bdj4",         no_argument,      NULL,   'B' },
    { "bdj4playerui", no_argument,      NULL,   0 },
    { "bdj4starterui", no_argument,     NULL,   0 },
    { "bdj4updater",  no_argument,      NULL,   0 },
    { "main",         no_argument,      NULL,   0 },
    { "playerui",     no_argument,      NULL,   0 },
    { "testsuite",    no_argument,      NULL,   0 },
    /* bdj4 loader options to ignore */
    { "debugself",    no_argument,      NULL,   0 },
    { "msys",         no_argument,      NULL,   0 },
    { "theme",        required_argument,NULL,   0 },
    /* normal options */
    { "profile",      required_argument,NULL,   'p' },
    { "debug",        required_argument,NULL,   'd' },
    { "startlog",     no_argument,      NULL,   's' },
    { "logstderr",    no_argument,      NULL,   'l' },
    { "hidemarquee",  no_argument,      NULL,   'h' },
    /* debug options */
    { "nostart",      no_argument,      NULL,   'n' },
    { "nomarquee",    no_argument,      NULL,   'm' },
    { "nodetach",     no_argument,      NULL,   'N' },
    { "wait",         no_argument,      NULL,   'w' },
    /* bdj4updater */
    { "newinstall",   no_argument,      NULL,   0 },
    { "converted",    no_argument,      NULL,   0 },
    { "musicdir",     required_argument,NULL,   0 },
    /* dbupdate options */
    { "rebuild",      no_argument,      NULL,   'R' },
    { "checknew",     no_argument,      NULL,   'C' },
    { "progress",     no_argument,      NULL,   'P' },
    { "reorganize",   no_argument,      NULL,   'O' },
    { "updfromtags",  no_argument,      NULL,   'u' },
    { "writetags",    no_argument,      NULL,   'W' },
    { "dbtopdir",     required_argument,NULL,   'D' },
    { "cli",          no_argument,      NULL,   'c' },
    /* test suite options */
    { "runsection",   required_argument,NULL,   'S' },
    { "runtest",      required_argument,NULL,   'T' },
    { "starttest",    required_argument,NULL,   'U' },
    { "verbose",      no_argument,      NULL,   'V' },
    { NULL,           0,                NULL,   0 }
  };

  mstimestart (&mt);
  sRandom ();
  sysvarsInit (argv[0]);
  localeInit ();
  bdjvarsInit ();
  audiotagInit ();

  optind = 0;
  while ((c = getopt_long_only (argc, argv, "BCPOUWcld:p:mnNRsh", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      case 'C': {
        flags |= BDJ4_DB_CHECK_NEW;
        break;
      }
      case 'P': {
        flags |= BDJ4_DB_PROGRESS;
        break;
      }
      case 'O': {
        flags |= BDJ4_DB_REORG;
        break;
      }
      case 'u': {
        flags |= BDJ4_DB_UPD_FROM_TAGS;
        break;
      }
      case 'W': {
        flags |= BDJ4_DB_WRITE_TAGS;
        break;
      }
      case 'D': {
        if (optarg) {
          logMsg (LOG_DBG, LOG_BASIC, "set dbtopdir %s", optarg);
          bdjvarsSetStr (BDJV_DB_TOP_DIR, optarg);
        }
        break;
      }
      case 'l': {
        logSetLevel (LOG_DBG, LOG_STDERR, tag);
        logSetLevel (LOG_ERR, LOG_STDERR, tag);
        break;
      }
      case 'd': {
        if (optarg) {
          loglevel = (loglevel_t) atoi (optarg);
          loglevelset = true;
        }
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarsSetNum (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      case 'm': {
        flags |= BDJ4_INIT_NO_MARQUEE;
        break;
      }
      case 'n': {
        flags |= BDJ4_INIT_NO_START;
        break;
      }
      case 'N': {
        flags |= BDJ4_INIT_NO_DETACH;
        break;
      }
      case 'w': {
        flags |= BDJ4_INIT_WAIT;
        break;
      }
      case 'R': {
        flags |= BDJ4_DB_REBUILD;
        break;
      }
      case 's': {
        startlog = true;
        break;
      }
      case 'h': {
        flags |= BDJ4_INIT_HIDE_MARQUEE;
        break;
      }
      case 'S': {
        if (optarg) {
          bdjvarsSetStr (BDJV_TS_SECTION, optarg);
        }
        flags |= BDJ4_TS_RUNSECTION;
        break;
      }
      case 'T': {
        if (optarg) {
          bdjvarsSetStr (BDJV_TS_TEST, optarg);
        }
        flags |= BDJ4_TS_RUNTEST;
        break;
      }
      case 'U': {
        if (optarg) {
          bdjvarsSetStr (BDJV_TS_TEST, optarg);
        }
        flags |= BDJ4_TS_STARTTEST;
        break;
      }
      case 'V': {
        flags |= BDJ4_TS_VERBOSE;
        break;
      }
      case 'c': {
        flags |= BDJ4_DB_CLI;
        break;
      }
      default: {
        break;
      }
    }
  }

  if (! isbdj4) {
    fprintf (stderr, "bdj4init: not started with launcher\n");
    exit (1);
  }

  bdjvarsAdjustPorts ();

  if ((flags & BDJ4_INIT_NO_LOCK) != BDJ4_INIT_NO_LOCK) {
    rc = lockAcquire (lockName (route), PATHBLD_MP_USEIDX);
    count = 0;
    while (rc < 0) {
      ++count;
      if (count > 10) {
        fprintf (stderr, "Unable to acquire lock: %s\n", lockName (route));
        bdj4shutdown (route, NULL);
        exit (1);
      }
      rc = lockAcquire (lockName (route), PATHBLD_MP_USEIDX);
    }
  }

  if (chdir (sysvarsGetStr (SV_BDJ4DATATOPDIR)) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvarsGetStr (SV_BDJ4DATATOPDIR));
    exit (1);
  }

  pathbldMakePath (tbuff, sizeof (tbuff),
      "", "", PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  if (! fileopIsDirectory (tbuff)) {
    sysvarsSetNum (SVL_BDJIDX, 0);
  }

  bdjoptInit ();
  tagdefInit ();

  if (! loglevelset) {
    loglevel = bdjoptGetNum (OPT_G_DEBUGLVL);
  }

  /* re-use the lock name as the program name */
  if (startlog || route == ROUTE_STARTERUI) {
    logStart (lockName (route), tag, loglevel);
  } else {
    logStartAppend (lockName (route), tag, loglevel);
  }
  logProcBegin (LOG_PROC, "bdj4startup");
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", sysvarsGetNum (SVL_BDJIDX));
  if (startlog || route == ROUTE_STARTERUI) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "locale: %s", sysvarsGetStr (SV_LOCALE));
    logMsg (LOG_SESS, LOG_IMPORTANT, "locale-short: %s", sysvarsGetStr (SV_LOCALE_SHORT));
    logMsg (LOG_SESS, LOG_IMPORTANT, "locale-system: %s", sysvarsGetStr (SV_LOCALE_SYSTEM));
  }

  if ((flags & BDJ4_INIT_NO_DATAFILE_LOAD) != BDJ4_INIT_NO_DATAFILE_LOAD) {
    rc = bdjvarsdfloadInit ();
    if (rc < 0) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "Unable to load all data files");
      fprintf (stderr, "Unable to load all data files\n");
      exit (1);
    }
  }

  bdjoptSetNum (OPT_G_DEBUGLVL, loglevel);

  if ((flags & BDJ4_INIT_NO_DB_LOAD) != BDJ4_INIT_NO_DB_LOAD &&
      musicdb != NULL) {
    mstimestart (&dbmt);
    logMsg (LOG_SESS, LOG_IMPORTANT, "Database read: started");
    pathbldMakePath (tbuff, sizeof (tbuff),
        MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_DATA);
    *musicdb = dbOpen (tbuff);
    logMsg (LOG_SESS, LOG_IMPORTANT, "Database read: %ld items in %ld ms", dbCount(*musicdb), mstimeend (&dbmt));
  }
  logMsg (LOG_SESS, LOG_IMPORTANT, "Total init time: %ld ms", mstimeend (&mt));

  logProcEnd (LOG_PROC, "bdj4startup", "");
  return flags;
}

musicdb_t *
bdj4ReloadDatabase (musicdb_t *musicdb)
{
  mstime_t    dbmt;
  char        tbuff [MAXPATHLEN];

  mstimestart (&dbmt);
  if (musicdb != NULL) {
    dbClose (musicdb);
  }
  logMsg (LOG_DBG, LOG_IMPORTANT, "Database read: started");
  pathbldMakePath (tbuff, sizeof (tbuff),
      MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_DATA);
  musicdb = dbOpen (tbuff);
  logMsg (LOG_DBG, LOG_IMPORTANT, "Database read: %ld items in %ld ms", dbCount(musicdb), mstimeend (&dbmt));
  return musicdb;
}

void
bdj4shutdown (bdjmsgroute_t route, musicdb_t *musicdb)
{
  mstime_t       mt;

  logProcBegin (LOG_PROC, "bdj4shutdown");

  if (strcmp (sysvarsGetStr (SV_BDJ4_RELEASELEVEL), "alpha") == 0) {
    char    tbuff [MAXPATHLEN];

    pathbldMakePath (tbuff, sizeof (tbuff),
        "core", "", PATHBLD_MP_MAINDIR);
    if (fileopFileExists (tbuff)) {
      fprintf (stderr, "== core file exists\n");
    }
  }

  mstimestart (&mt);
  bdjoptCleanup ();
  if (musicdb != NULL) {
    dbClose (musicdb);
  }
  bdjvarsdfloadCleanup ();
  bdjvarsCleanup ();
  tagdefCleanup ();
  audiotagCleanup ();
  localeCleanup ();
  logMsg (LOG_SESS, LOG_IMPORTANT, "init cleanup time: %ld ms", mstimeend (&mt));
  if (route != ROUTE_NONE) {
    lockRelease (lockName (route), PATHBLD_MP_USEIDX);
  }
  logProcEnd (LOG_PROC, "bdj4shutdown", "");
}
