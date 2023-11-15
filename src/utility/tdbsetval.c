/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#include "audiotag.h"
#include "bdj4.h"
#include "bdj4arg.h"
#include "bdjopt.h"
#include "bdjvarsdfload.h"
#include "fileop.h"
#include "localeutil.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "slist.h"
#include "song.h"
#include "sysvars.h"
#include "tagdef.h"

int
main (int argc, char *argv [])
{
  musicdb_t   *db;
  bool        isbdj4 = false;
  const char  *dbfn = NULL;
  const char  *tagname = NULL;
  const char  *valuestr = NULL;
  char        *maxcountstr = NULL;
  long        maxcount = -1;
  long        count = 0;
  int         c;
  int         option_index;
  song_t      *song;
  int         grc = 0;
  int         argcount = 0;
  slistidx_t  dbiteridx;
  dbidx_t     dbkey;
  int         tagidx = -1;
  bdj4arg_t   *bdj4arg;
  const char  *targ;

  static struct option bdj_options [] = {
    { "bdj4",         no_argument,      NULL,   'B' },
    { "tdbsetval",    no_argument,      NULL,   0 },
    { "debugself",    no_argument,      NULL,   0 },
    { "wait",         no_argument,      NULL,   0, },
    { "nodetach",     no_argument,      NULL,   0, },
    { "verbose",      no_argument,      NULL,   'V', },
    { "origcwd",      required_argument,  NULL,   0 },
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("tdbs");
#endif

  bdj4arg = bdj4argInit (argc, argv);

  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "BV", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      default: {
        break;
      }
    }
  }

  if (! isbdj4) {
    fprintf (stderr, "not started with launcher\n");
    bdj4argCleanup (bdj4arg);
    return 1;
  }

  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ);
  localeInit ();
  bdjoptInit ();
  tagdefInit ();
  audiotagInit ();

  bdjvarsdfloadInit ();

  for (int i = optind; i < argc; ++i) {
    if (argcount == 0 && dbfn == NULL) {
      targ = bdj4argGet (bdj4arg, i, argv [i]);
      dbfn = targ;
    }
    if (argcount == 1 && tagname == NULL) {
      targ = bdj4argGet (bdj4arg, i, argv [i]);
      tagname = targ;
      tagidx = tagdefLookup (tagname);
      if (tagidx < 0) {
        fprintf (stderr, "unknown tag name %s\n", tagname);
        bdj4argCleanup (bdj4arg);
        exit (1);
      }
    }
    if (argcount == 2 && valuestr == NULL) {
      targ = bdj4argGet (bdj4arg, i, argv [i]);
      valuestr = targ;
    }
    if (argcount == 3 && maxcountstr == NULL) {
      maxcountstr = argv [i];
      maxcount = atol (maxcountstr);
    }
    argcount++;
  }

  if (argcount < 3) {
    fprintf (stderr, "Usage: dbdump <db-a> <tagname> <value> [<count>](%d)\n", argcount);
    bdj4argCleanup (bdj4arg);
    return 1;
  }
  if (dbfn == NULL || ! fileopFileExists (dbfn)) {
    fprintf (stderr, "no file %s\n", dbfn);
    bdj4argCleanup (bdj4arg);
    return 1;
  }

  db = dbOpen (dbfn);
  dbDisableLastUpdateTime (db);
  dbStartBatch (db);

  if (db == NULL) {
    fprintf (stderr, "unable to open %s\n", dbfn);
    bdj4argCleanup (bdj4arg);
    return 1;
  }

  logStart ("tdbsetval", "tdbs",
      LOG_IMPORTANT | LOG_BASIC | LOG_INFO | LOG_MSGS | LOG_ACTIONS | LOG_DB);

  dbStartIterator (db, &dbiteridx);
  while ((song = dbIterate (db, &dbkey, &dbiteridx)) != NULL) {
    if (song != NULL) {
      if (tagdefs [tagidx].valueType == VALUE_NUM) {
        songSetNum (song, tagidx, atol (valuestr));
      } else if (tagdefs [tagidx].valueType == VALUE_STR) {
        songSetStr (song, tagidx, valuestr);
      }
      dbWriteSong (db, song);
    }
    ++count;
    if (maxcount > 0 && count >= maxcount) {
      break;
    }
  }

  dbEndBatch (db);
  dbClose (db);

  audiotagCleanup ();
  bdjvarsdfloadCleanup ();
  tagdefCleanup ();
  bdjoptCleanup ();
  localeCleanup ();
  logEnd ();
  bdj4argCleanup (bdj4arg);
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return grc;
}

