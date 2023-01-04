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
#include "musicdb.h"
#include "slist.h"
#include "song.h"
#include "sysvars.h"
#include "tagdef.h"

enum {
  DB_A,
  DB_B,
  DB_MAX,
};

int
main (int argc, char *argv [])
{
  musicdb_t   *db [DB_MAX];
  bool        isbdj4 = false;
  bool        verbose = false;
  const char  *dbfn [DB_MAX];
  int         c;
  int         option_index;
  dbidx_t     count [DB_MAX];
  slistidx_t  dbiteridx [DB_MAX];
  song_t      *song [DB_MAX];
  int         grc = 0;
  int         argcount = 0;
  dbidx_t     dbidx [DB_MAX];
  loglevel_t  loglevel = LOG_IMPORTANT | LOG_MAIN;
  bool        loglevelset = false;

  static struct option bdj_options [] = {
    { "bdj4",         no_argument,        NULL,   'B' },
    { "tdbcompare",   no_argument,        NULL,   0 },
    { "msys",         no_argument,        NULL,   0 },
    { "debug",        required_argument,  NULL,   'd' },
    { "debugself",    no_argument,        NULL,   0 },
    { "nodetach",     no_argument,        NULL,   0, },
    { "verbose",      no_argument,        NULL,   'V', },
  };

  while ((c = getopt_long_only (argc, argv, "B3Vd", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      case 'V': {
        verbose = true;
        break;
      }
      case 'd': {
        if (optarg) {
          loglevel = atol (optarg);
          loglevelset = true;
        }
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
  audiotagInit ();

  bdjvarsdfloadInit ();

  if (! loglevelset) {
    loglevel = bdjoptGetNum (OPT_G_DEBUGLVL);
  }
  logStartAppend ("tdbcompare", "td", loglevel);

  for (int i = 0; i < DB_MAX; ++i) {
    dbfn [i] = NULL;
    db [i] = NULL;
    count [i] = 0;
    song [i] = NULL;
  }

  for (int i = 1; i < argc; ++i) {
    if (strncmp (argv [i], "--", 2) == 0) {
      continue;
    }
    dbfn [argcount] = argv [i];
    argcount++;
  }

  if (argcount < 2) {
    fprintf (stderr, "Usage: dbcompare <db-a> <db-b> (%d)\n", argcount);
    return 1;
  }
  for (int i = 0; i < DB_MAX; ++i) {
    if (dbfn [i] == NULL || ! fileopFileExists (dbfn [i])) {
      fprintf (stderr, "no file %s\n", dbfn [i]);
      return 1;
    }
  }

  for (int i = 0; i < DB_MAX; ++i) {
    db [i] = dbOpen (dbfn [i]);
    if (db [i] == NULL) {
      fprintf (stderr, "unable to open %s\n", dbfn [i]);
      return 1;
    }
    count [i] = dbCount (db [i]);
  }

  if (count [DB_A] != count [DB_B]) {
    fprintf (stderr, "  dbcompare: count mismatch %d/%d\n", count [DB_A], count [DB_B]);
    grc = 1;
  }

  dbStartIterator (db [DB_A], &dbiteridx [DB_A]);
  while ((song [DB_A] = dbIterate (db [DB_A], &dbidx [DB_A], &dbiteridx [DB_A])) != NULL) {
    slist_t     *taglist [DB_MAX];
    slistidx_t  tagiteridx [DB_MAX];
    char        *tag [DB_MAX];
    const char  *fn;


    for (int i = 0; i < DB_MAX; ++i) {
      taglist [i] = NULL;
      tag [i] = NULL;
    }

    fn = songGetStr (song [DB_A], TAG_FILE);
    if (verbose) {
      fprintf (stderr, "  -- %s\n", fn);
    }

    song [DB_B] = dbGetByName (db [DB_B], fn);
    if (song [DB_B] == NULL) {
      fprintf (stderr, "    song %s not found\n", fn);
      grc = 1;
      continue;
    }

    for (int i = 0; i < DB_MAX; ++i) {
      taglist [i] = songTagList (song [i]);
    }

    if (slistGetCount (taglist [DB_A]) != slistGetCount (taglist [DB_B])) {
      fprintf (stderr, "    song tag count mismatch %d/%d\n",
          slistGetCount (taglist [DB_A]), slistGetCount (taglist [DB_B]));
      grc = 1;
      continue;
    }

    slistStartIterator (taglist [DB_A], &tagiteridx [DB_A]);
    while ((tag [DB_A] = slistIterateKey (taglist [DB_A], &tagiteridx [DB_A])) != NULL) {
      char  *val [DB_MAX];

      if (strcmp (tag [DB_A], tagdefs [TAG_DBADDDATE].tag) == 0) {
        continue;
      }
      if (strcmp (tag [DB_A], tagdefs [TAG_LAST_UPDATED].tag) == 0) {
        continue;
      }
      if (strcmp (tag [DB_A], tagdefs [TAG_RRN].tag) == 0) {
        continue;
      }
      if (strcmp (tag [DB_A], tagdefs [TAG_DBIDX].tag) == 0) {
        continue;
      }

      for (int i = 0; i < DB_MAX; ++i) {
        val [i] = slistGetStr (taglist [i], tag [DB_A]);
      }

      if (val [DB_A] == NULL && val [DB_B] != NULL) {
        fprintf (stderr, "    null tag %s mismatch (null)/%s\n", tag [DB_A], val [DB_B]);
        grc = 1;
      }
      if (val [DB_A] != NULL && val [DB_B] == NULL) {
        fprintf (stderr, "    null tag %s mismatch %s/(null)\n", tag [DB_A], val [DB_A]);
        grc = 1;
      }
      if (val [DB_A] != NULL && val [DB_B] != NULL &&
          strcmp (val [DB_A], val [DB_B]) != 0) {
        fprintf (stderr, "    tag %s mismatch %s/%s\n", tag [DB_A], val [DB_A], val [DB_B]);
        grc = 1;
      }
    }

    for (int i = 0; i < DB_MAX; ++i) {
      slistFree (taglist [i]);
      dataFree (tag [i]);
    }
  }

  for (int i = 0; i < DB_MAX; ++i) {
    dbClose (db [i]);
  }

  audiotagCleanup ();
  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
  localeCleanup ();

  return grc;
}

