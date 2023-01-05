/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

#include "audiotag.h"
#include "bdj4.h"
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
  bool        verbose = false;
  const char  *dbfn;
  const char  *songnm;
  int         c;
  int         option_index;
  song_t      *song;
  int         grc = 0;
  int         argcount = 0;

  static struct option bdj_options [] = {
    { "bdj4",         no_argument,      NULL,   'B' },
    { "tdbdump",      no_argument,      NULL,   0 },
    { "debugself",    no_argument,      NULL,   0 },
    { "msys",         no_argument,        NULL,   0 },
    { "wait",         no_argument,      NULL,   0, },
    { "nodetach",     no_argument,      NULL,   0, },
    { "verbose",      no_argument,      NULL,   'V', },
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("tdbd");
#endif

  while ((c = getopt_long_only (argc, argv, "BV", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      case 'V': {
        verbose = true;
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
  tagdefInit ();
  audiotagInit ();

  bdjvarsdfloadInit ();

  dbfn = NULL;
  songnm = NULL;
  for (int i = 1; i < argc; ++i) {
    if (strncmp (argv [i], "--", 2) == 0) {
      continue;
    }
    if (dbfn == NULL) {
      dbfn = argv [i];
    }
    songnm = argv [i];
    argcount++;
  }

  if (argcount < 2) {
    fprintf (stderr, "Usage: dbdump <db-a> <song-name> (%d)\n", argcount);
    return 1;
  }
  if (dbfn == NULL || ! fileopFileExists (dbfn)) {
    fprintf (stderr, "no file %s\n", dbfn);
    return 1;
  }

  db = dbOpen (dbfn);
  if (db == NULL) {
    fprintf (stderr, "unable to open %s\n", dbfn);
    return 1;
  }

  song = dbGetByName (db, songnm);
  if (song == NULL) {
    fprintf (stderr, "song %s not found\n", songnm);
    grc = 1;
  }

  if (song != NULL) {
    slist_t     *taglist;
    const char  *tag;
    slistidx_t  tagiteridx;

    taglist = songTagList (song);

    slistStartIterator (taglist, &tagiteridx);
    while ((tag = slistIterateKey (taglist, &tagiteridx)) != NULL) {
      char  *val;

      if (! verbose &&
          (strcmp (tag, "DBADDDATE") == 0 ||
          strcmp (tag, "FILE") == 0 ||
          strcmp (tag, "LASTUPDATED") == 0 ||
          strcmp (tag, "RRN") == 0)) {
        continue;
      }

      val = slistGetStr (taglist, tag);
      if (val != NULL && *val) {
        fprintf (stdout, "%-20s %s\n", tag, val);
      }
    }

    slistFree (taglist);
  }

  dbClose (db);

  audiotagCleanup ();
  bdjvarsdfloadCleanup ();
  tagdefCleanup ();
  bdjoptCleanup ();
  localeCleanup ();
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return grc;
}

