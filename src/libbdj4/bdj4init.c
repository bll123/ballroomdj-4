/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "audiosrc.h"
#include "audiotag.h"
#include "bdj4.h"
#include "bdj4arg.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdfload.h"
#include "bdjvars.h"
#include "fileop.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "musicdb.h"
#include "pathbld.h"
#include "osrandom.h"
#include "osdirutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"

loglevel_t
bdj4startup (int argc, char *argv[], musicdb_t **musicdb,
    char *tag, bdjmsgroute_t route, uint32_t *flags)
{
  mstime_t    mt;
  mstime_t    dbmt;
  int         c = 0;
  int         rc = 0;
  int         count = 0;
  int         option_index = 0;
  char        tbuff [MAXPATHLEN];
  bdj4arg_t   *bdj4arg;
  const char  *targ;
  loglevel_t  loglevel = 0;
  bool        loglevelset = false;
  bool        isbdj4 = false;

  static struct option bdj_options [] = {
    { "bdj4altinst",    no_argument,        NULL,   0 },
    { "bdj4bpmcounter", no_argument,        NULL,   0 },
    { "bdj4configui",   no_argument,        NULL,   0 },
    { "bdj4dbupdate",   no_argument,        NULL,   0 },
    { "bdj4helperui",   no_argument,        NULL,   0 },
    { "bdj4main",       no_argument,        NULL,   0 },
    { "bdj4manageui",   no_argument,        NULL,   0 },
    { "bdj4",           no_argument,        NULL,   'B' },
    { "bdj4playerui",   no_argument,        NULL,   0 },
    { "bdj4podcastupd", no_argument,        NULL,   0 },
    { "bdj4starterui",  no_argument,        NULL,   0 },
    { "bdj4updater",    no_argument,        NULL,   0 },
    { "main",           no_argument,        NULL,   0 },
    { "playerui",       no_argument,        NULL,   0 },
    { "testsuite",      no_argument,        NULL,   0 },
    /* bdj4 loader options to ignore */
    { "debugself",      no_argument,        NULL,   0 },
    { "scale",          required_argument,  NULL,   0 },
    { "theme",          required_argument,  NULL,   0 },
    /* normal options */
    { "debug",          required_argument,  NULL,   'd' },
    { "hidemarquee",    no_argument,        NULL,   'h' },
    { "locale",         required_argument,  NULL,   'L' },
    { "profile",        required_argument,  NULL,   'p' },
    /* debug options */
    { "nostart",        no_argument,        NULL,   'n' },
    { "nomarquee",      no_argument,        NULL,   'm' },
    { "nodetach",       no_argument,        NULL,   'N' },
    { "pli",            required_argument,  NULL,   0 },
    { "wait",           no_argument,        NULL,   'w' },
    /* starter */
    { "datatopdir",     required_argument,  NULL,   't' },
    { "nopodcastupd",   no_argument,        NULL,   8 },
    /* installers */
    { "unattended",     no_argument,        NULL,   1 },
    { "reinstall",      no_argument,        NULL,   2 },
    { "datatopdir",     required_argument,  NULL,   3 },
    { "targetdir",      required_argument,  NULL,   4 },
    { "name",           required_argument,  NULL,   5 },
    /* bdj4updater */
    { "newinstall",     no_argument,        NULL,   6 },
    { "convert",        no_argument,        NULL,   7 },
    /* dbupdate options */
    { "rebuild",        no_argument,        NULL,   'R' },
    { "checknew",       no_argument,        NULL,   'C' },
    { "compact",        no_argument,        NULL,   127 },
    { "musicdir",       required_argument,  NULL,   'D' },
    { "reorganize",     no_argument,        NULL,   'O' },
    { "updfromtags",    no_argument,        NULL,   'u' },
    { "updfromitunes",  no_argument,        NULL,   'I' },
    { "writetags",      no_argument,        NULL,   'W' },
    /* generic options, some used by dbupdate, test suite */
    { "progress",       no_argument,        NULL,   'P' },
    { "cli",            no_argument,        NULL,   'c' },
    { "verbose",        no_argument,        NULL,   'V' },
    { "quiet",          no_argument,        NULL,   'Q' },
    { "origcwd",        required_argument,  NULL,   0 },
    /* test suite options */
    { "runsection",     required_argument,  NULL,   'S' },
    { "runtest",        required_argument,  NULL,   'T' },
    { "starttest",      required_argument,  NULL,   'U' },
    { "outfile",        required_argument,  NULL,   0 },
    { "infile",         required_argument,  NULL,   0 },
    { NULL,             0,                  NULL,   0 }
  };

  bdj4arg = bdj4argInit (argc, argv);

  mstimestart (&mt);
  sRandom ();
  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ, SYSVARS_FLAG_ALL);
  localeInit ();
  bdjvarsInit ();

  optind = 0;
  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "BChPOUWcld:p:mnNRst:T", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      case 't': {
        targ = bdj4argGet (bdj4arg, optind - 1, optarg);
        if (fileopIsDirectory (targ)) {
          sysvarsSetStr (SV_BDJ4_DIR_DATATOP, targ);
          sysvarsSetNum (SVL_DATAPATH, SYSVARS_DATAPATH_ALT);
        }
        break;
      }
      case 'C': {
        *flags |= BDJ4_ARG_DB_CHECK_NEW;
        break;
      }
      case 1: {
        *flags |= BDJ4_ARG_INST_UNATTENDED;
        break;
      }
      case 2: {
        *flags |= BDJ4_ARG_INST_REINSTALL;
        break;
      }
      case 3: {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          logMsg (LOG_DBG, LOG_BASIC, "set datatop %s", targ);
          bdjvarsSetStr (BDJV_INST_DATATOP, targ);
        }
        break;
      }
      case 4: {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          logMsg (LOG_DBG, LOG_BASIC, "set target %s", targ);
          bdjvarsSetStr (BDJV_INST_TARGET, targ);
        }
        break;
      }
      case 5: {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          logMsg (LOG_DBG, LOG_BASIC, "set name %s", targ);
          bdjvarsSetStr (BDJV_INST_NAME, targ);
        }
        break;
      }
      case 6: {
        *flags |= BDJ4_ARG_UPD_NEW;
        break;
      }
      case 7: {
        *flags |= BDJ4_ARG_UPD_CONVERT;
        break;
      }
      case 8: {
        *flags |= BDJ4_ARG_NO_PODCAST_UPD;
        break;
      }
      case 127: {
        *flags |= BDJ4_ARG_DB_COMPACT;
        break;
      }
      case 'P': {
        *flags |= BDJ4_ARG_PROGRESS;
        break;
      }
      case 'O': {
        *flags |= BDJ4_ARG_DB_REORG;
        break;
      }
      case 'u': {
        *flags |= BDJ4_ARG_DB_UPD_FROM_TAGS;
        break;
      }
      case 'I': {
        *flags |= BDJ4_ARG_DB_UPD_FROM_ITUNES;
        break;
      }
      case 'W': {
        *flags |= BDJ4_ARG_DB_WRITE_TAGS;
        break;
      }
      case 'D': {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          logMsg (LOG_DBG, LOG_BASIC, "set musicdir %s", targ);
          bdjvarsSetStr (BDJV_MUSIC_DIR, targ);
        }
        break;
      }
      case 'd': {
        if (optarg != NULL) {
          loglevel = (loglevel_t) atol (optarg);
          loglevelset = true;
        }
        break;
      }
      case 'p': {
        if (optarg != NULL) {
          sysvarsSetNum (SVL_PROFILE_IDX, atol (optarg));
        }
        break;
      }
      case 'L': {
        if (optarg != NULL) {
          sysvarsSetStr (SV_LOCALE, optarg);
          snprintf (tbuff, sizeof (tbuff), "%.2s", optarg);
          sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
          sysvarsSetNum (SVL_LOCALE_SET, 1);
          localeSetup ();
        }
        break;
      }
      case 'm': {
        *flags |= BDJ4_INIT_NO_MARQUEE;
        break;
      }
      case 'n': {
        *flags |= BDJ4_INIT_NO_START;
        break;
      }
      case 'N': {
        *flags |= BDJ4_INIT_NO_DETACH;
        break;
      }
      case 'w': {
        *flags |= BDJ4_INIT_WAIT;
        break;
      }
      case 'R': {
        *flags |= BDJ4_ARG_DB_REBUILD;
        break;
      }
      case 'h': {
        *flags |= BDJ4_INIT_HIDE_MARQUEE;
        break;
      }
      case 'S': {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          bdjvarsSetStr (BDJV_TS_SECTION, targ);
          *flags |= BDJ4_ARG_TS_RUNSECTION;
        }
        break;
      }
      case 'T': {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          bdjvarsSetStr (BDJV_TS_TEST, targ);
          *flags |= BDJ4_ARG_TS_RUNTEST;
        }
        break;
      }
      case 'U': {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          bdjvarsSetStr (BDJV_TS_TEST, targ);
          *flags |= BDJ4_ARG_TS_STARTTEST;
        }
        break;
      }
      case 'V': {
        *flags |= BDJ4_ARG_VERBOSE;
        break;
      }
      case 'Q': {
        *flags |= BDJ4_ARG_QUIET;
        break;
      }
      case 'c': {
        *flags |= BDJ4_ARG_CLI;
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

  bdjvarsUpdateData ();

  if ((*flags & BDJ4_INIT_NO_LOCK) != BDJ4_INIT_NO_LOCK) {
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

  if (fileopIsDirectory (sysvarsGetStr (SV_BDJ4_DIR_DATATOP))) {
    if (osChangeDir (sysvarsGetStr (SV_BDJ4_DIR_DATATOP)) < 0) {
      fprintf (stderr, "Unable to chdir: %s\n", sysvarsGetStr (SV_BDJ4_DIR_DATATOP));
      exit (1);
    }
  }

  pathbldMakePath (tbuff, sizeof (tbuff),
      "", "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  if (! fileopIsDirectory (tbuff)) {
    sysvarsSetNum (SVL_PROFILE_IDX, 0);
  }

  bdjoptInit ();

  if (! loglevelset) {
    loglevel = bdjoptGetNum (OPT_G_DEBUGLVL);
  }

  if ((*flags & BDJ4_INIT_NO_LOG) != BDJ4_INIT_NO_LOG) {
    /* re-use the lock name as the program name */
    if (route == ROUTE_STARTERUI) {
      logStart (lockName (route), tag, loglevel);
    } else {
      logStartAppend (lockName (route), tag, loglevel);
    }
  }

  logProcBegin ();
  logMsg (LOG_SESS, LOG_IMPORTANT, "using profile %" PRId64, sysvarsGetNum (SVL_PROFILE_IDX));
  if (route == ROUTE_STARTERUI) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "locale: %s", sysvarsGetStr (SV_LOCALE));
    logMsg (LOG_SESS, LOG_IMPORTANT, "locale-short: %s", sysvarsGetStr (SV_LOCALE_SHORT));
    logMsg (LOG_SESS, LOG_IMPORTANT, "locale-system: %s", sysvarsGetStr (SV_LOCALE_SYSTEM));
  }

  if (fileopIsDirectory (sysvarsGetStr (SV_BDJ4_DIR_DATATOP))) {
    if ((*flags & BDJ4_INIT_NO_DATAFILE_LOAD) != BDJ4_INIT_NO_DATAFILE_LOAD) {
      rc = bdjvarsdfloadInit ();
      if (rc < 0) {
        logMsg (LOG_SESS, LOG_IMPORTANT, "unable to load all data files");
        fprintf (stderr, "unable to load all data files\n");
        exit (1);
      }
    }
  }

  bdjoptSetNum (OPT_G_DEBUGLVL, loglevel);

  tagdefInit ();
  audiotagInit ();
  audiosrcInit ();

  if ((*flags & BDJ4_INIT_NO_DB_LOAD) != BDJ4_INIT_NO_DB_LOAD &&
      musicdb != NULL) {
    mstimestart (&dbmt);
    logMsg (LOG_SESS, LOG_IMPORTANT, "database read: started");
    pathbldMakePath (tbuff, sizeof (tbuff),
        MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_DREL_DATA);
    *musicdb = dbOpen (tbuff);
    logMsg (LOG_SESS, LOG_IMPORTANT, "database read: %" PRId32 " items in %" PRId64 " ms", dbCount(*musicdb), (int64_t) mstimeend (&dbmt));
  }
  logMsg (LOG_SESS, LOG_IMPORTANT, "total init time: %" PRId64 " ms", (int64_t) mstimeend (&mt));

  bdj4argCleanup (bdj4arg);
  logProcEnd ("");
  return loglevel;
}

musicdb_t *
bdj4ReloadDatabase (musicdb_t *musicdb)
{
  mstime_t    dbmt;
  char        tbuff [MAXPATHLEN];

  mstimestart (&dbmt);
  dbClose (musicdb);
  logMsg (LOG_DBG, LOG_IMPORTANT, "database read: started");
  pathbldMakePath (tbuff, sizeof (tbuff),
      MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_DREL_DATA);
  musicdb = dbOpen (tbuff);
  logMsg (LOG_DBG, LOG_IMPORTANT, "database read: %" PRId32 " items in %" PRId64 " ms", dbCount(musicdb), (int64_t) mstimeend (&dbmt));
  return musicdb;
}

void
bdj4shutdown (bdjmsgroute_t route, musicdb_t *musicdb)
{
  mstime_t       mt;

  logProcBegin ();

  if (strcmp (sysvarsGetStr (SV_BDJ4_DEVELOPMENT), "dev") == 0) {
    char    tbuff [MAXPATHLEN];

    pathbldMakePath (tbuff, sizeof (tbuff),
        "core", "", PATHBLD_MP_DIR_MAIN);
    if (fileopFileExists (tbuff)) {
      fprintf (stderr, "== core file exists\n");
    }
  }

  mstimestart (&mt);
  audiosrcCleanup ();
  tagdefCleanup ();
  bdjoptCleanup ();
  dbClose (musicdb);
  bdjvarsdfloadCleanup ();
  audiotagCleanup ();
  bdjvarsCleanup ();
  localeCleanup ();
  logMsg (LOG_SESS, LOG_IMPORTANT, "init cleanup time: %" PRId64 " ms", (int64_t) mstimeend (&mt));
  if (route != ROUTE_NONE) {
    lockRelease (lockName (route), PATHBLD_MP_USEIDX);
  }
  logProcEnd ("");
}

