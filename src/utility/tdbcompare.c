/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>

#include "audiosrc.h"
#include "audiotag.h"
#include "bdj4.h"
#include "bdj4arg.h"
#include "bdjopt.h"
#include "bdjvars.h"
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

enum {
  DB_A,
  DB_B,
  DB_C,
  DB_D,
  DB_MAX,
};

int
main (int argc, char *argv [])
{
  musicdb_t   *db [DB_MAX];
  bool        isbdj4 = false;
  bool        verbose = false;
  const char  *dbfn [DB_MAX];
  int         dblocidx = 0;
  dbidx_t     totcount;
  int         c;
  int         option_index;
  dbidx_t     count [DB_MAX];
  slistidx_t  dbiteridx [DB_MAX];
  song_t      *song [DB_MAX];
  int         grc = 0;
  int         argcount = 0;
  dbidx_t     dbidx [DB_MAX];
  loglevel_t  loglevel = LOG_IMPORTANT | LOG_INFO;
  bool        loglevelset = false;
  bool        loclockcheck = true;
  bdj4arg_t   *bdj4arg;
  const char  *targ;

  static struct option bdj_options [] = {
    { "tdbcompare",   no_argument,        NULL,   0 },
    { "noloclockchk", no_argument,        NULL,   'l' },
    /* launcher options */
    { "bdj4",         no_argument,        NULL,   'B' },
    { "debugself",    no_argument,        NULL,   0 },
    { "nodetach",     no_argument,        NULL,   0, },
    { "origcwd",      required_argument,  NULL,   0 },
    { "pli",          required_argument,  NULL,   0, },
    { "wait",         no_argument,        NULL,   0, },
    /* standard stuff */
    { "debug",        required_argument,  NULL,   'd' },
    /* general args */
    { "verbose",      no_argument,        NULL,   'V', },
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("tdbc");
#endif

  bdj4arg = bdj4argInit (argc, argv);

  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "B3Vd", bdj_options, &option_index)) != -1) {
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
      case 'l': {
        loclockcheck = false;
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

  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ, SYSVARS_FLAG_ALL);
  localeInit ();
  bdjoptInit ();
  bdjvarsInit ();
  audiotagInit ();
  bdjvarsdfloadInit ();
  audiosrcInit ();

  if (! loglevelset) {
    loglevel = bdjoptGetNum (OPT_G_DEBUGLVL);
  }
  logStartAppend ("tdbcompare", "tdbc", loglevel);

  logMsg (LOG_DBG, LOG_IMPORTANT, "ati: %s", bdjoptGetStr (OPT_M_AUDIOTAG_INTFC));

  for (int i = 0; i < DB_MAX; ++i) {
    dbfn [i] = NULL;
    db [i] = NULL;
    count [i] = 0;
    song [i] = NULL;
  }

  for (int i = optind; i < argc; ++i) {
    if (argcount < DB_MAX) {
      targ = bdj4argGet (bdj4arg, i, argv [i]);
      dbfn [argcount] = targ;
    }
    argcount++;
  }

  if (argcount < 2) {
    fprintf (stderr, "Usage: dbcompare <db-a> {<db-b>...} (%d)\n", argcount);
    return 1;
  }

  for (int i = 0; i < argcount; ++i) {
    if (dbfn [i] == NULL || ! fileopFileExists (dbfn [i])) {
      fprintf (stderr, "tdbcompare: no file %s\n", dbfn [i]);
      return 1;
    }
  }

  totcount = 0;
  for (int i = 0; i < argcount; ++i) {
    db [i] = dbOpen (dbfn [i]);
    if (db [i] == NULL) {
      fprintf (stderr, "tdbcompare: unable to open %s\n", dbfn [i]);
      return 1;
    }
    count [i] = dbCount (db [i]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "count: %" PRId32 " %s", count [i], dbfn [i]);
    if (verbose) {
      fprintf (stderr, "count: %" PRId32 " %s\n", count [i], dbfn [i]);
    }
    if (i > DB_A) {
      totcount += count [i];
    }
  }

  if (count [DB_A] != totcount) {
    fprintf (stderr, "  tdbcompare: count mismatch /%" PRId32 "/%" PRId32 "/\n", count [DB_A], totcount);
    grc = 1;
  }

  dbStartIterator (db [DB_A], &dbiteridx [DB_A]);
  while ((song [DB_A] = dbIterate (db [DB_A], &dbidx [DB_A], &dbiteridx [DB_A])) != NULL) {
    slist_t     *taglist [DB_MAX];
    slistidx_t  tagiteridx [DB_MAX];
    const char  *tag [DB_MAX];
    const char  *fn;
    const char  *val;
    ssize_t     nval;

    for (int i = 0; i < DB_MAX; ++i) {
      taglist [i] = NULL;
      tag [i] = NULL;
    }

    fn = songGetStr (song [DB_A], TAG_URI);
    if (verbose) {
      fprintf (stderr, "  -- tdbcomp: %s\n", fn);
    }

    for (int i = DB_B; i < DB_MAX; ++i) {
      song [i] = dbGetByName (db [i], fn);
      if (song [i] != NULL) {
        dblocidx = i;
        break;
      }
    }

    if (song [dblocidx] == NULL) {
      fprintf (stderr, "    tdbcomp:a: song %s not found\n", fn);
      grc = 1;
      continue;
    }

    taglist [DB_A] = songTagList (song [DB_A]);
    taglist [dblocidx] = songTagList (song [dblocidx]);

    if (slistGetCount (taglist [DB_A]) != slistGetCount (taglist [dblocidx])) {
      fprintf (stderr, "    tdbcomp:b: song tag count mismatch /%" PRId32 "/%" PRId32 "/ %s\n",
          slistGetCount (taglist [DB_A]), slistGetCount (taglist [dblocidx]), fn);
      grc = 1;
      continue;
    }

    /* it's ok for dbadddate to be mismatched, but it must exist in both */
    nval = songGetNum (song [DB_A], TAG_DBADDDATE);
    if (nval < 0) {
      fprintf (stderr, "    tdbcomp:c: dbadddate missing in %d %s\n", DB_A, fn);
      grc = 1;
    }
    nval = songGetNum (song [dblocidx], TAG_DBADDDATE);
    if (nval < 0) {
      fprintf (stderr, "    tdbcomp:d: dbadddate missing in %d %s\n", dblocidx, fn);
      grc = 1;
    }

    /* it's ok for lastupdated to be mismatched, but it must exist in both */
    val = songGetStr (song [DB_A], TAG_LAST_UPDATED);
    if (val == NULL) {
      fprintf (stderr, "    tdbcomp:e: last-updated missing in %d %s\n", DB_A, fn);
      grc = 1;
    }
    val = songGetStr (song [dblocidx], TAG_LAST_UPDATED);
    if (val == NULL) {
      fprintf (stderr, "    tdbcomp:f: last-updated missing in %d %s\n", dblocidx, fn);
      grc = 1;
    }

    slistStartIterator (taglist [DB_A], &tagiteridx [DB_A]);
    while ((tag [DB_A] = slistIterateKey (taglist [DB_A], &tagiteridx [DB_A])) != NULL) {
      const char  *val [DB_MAX];

      if (strcmp (tag [DB_A], tagdefs [TAG_DBADDDATE].tag) == 0) {
        continue;
      }
      if (strcmp (tag [DB_A], tagdefs [TAG_TRACKNUMBER].tag) == 0) {
        /* track number no longer tested */
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
      if (! loclockcheck &&
          strcmp (tag [DB_A], tagdefs [TAG_DB_LOC_LOCK].tag) == 0) {
        continue;
      }
      if (strcmp (tag [DB_A], tagdefs [TAG_PREFIX_LEN].tag) == 0) {
        continue;
      }

      val [DB_A] = slistGetStr (taglist [DB_A], tag [DB_A]);
      val [dblocidx] = slistGetStr (taglist [dblocidx], tag [DB_A]);

      if (val [DB_A] == NULL && val [dblocidx] != NULL) {
        fprintf (stderr, "    tdbcomp:g: null tag %s mismatch /(null)/%s/ %s\n", tag [DB_A], val [dblocidx], fn);
        grc = 1;
      }
      if (val [DB_A] != NULL && val [dblocidx] == NULL) {
        fprintf (stderr, "    tdbcomp:h: null tag %s mismatch /%s/(null)/ %s\n", tag [DB_A], val [DB_A], fn);
        grc = 1;
      }
      if (val [DB_A] != NULL && strcmp (val [DB_A], "(null)") == 0) {
        fprintf (stderr, "    tdbcomp:i: tag %s has '(null)' string (db-a) %s\n", tag [DB_A], fn);
        grc = 1;
      }
      if (val [dblocidx] != NULL && strcmp (val [dblocidx], "(null)") == 0) {
        fprintf (stderr, "    tdbcomp:j: tag %s has '(null)' string (db-b) %s\n", tag [DB_A], fn);
        grc = 1;
      }

      if (strcmp (tag [DB_A], tagdefs [TAG_DURATION].tag) == 0) {
        if (val [DB_A] != NULL && val [dblocidx] != NULL) {
          int32_t     dura, durb;

          dura = atol (val [DB_A]);
          durb = atol (val [dblocidx]);
          if (abs (dura - durb) > 100) {
            fprintf (stderr, "    tdbcomp:k: tag %s mismatch /%s/%s/ %s\n", tag [DB_A], val [DB_A], val [dblocidx], fn);
            grc = 1;
          }
        }
      } else if (strcmp (tag [DB_A], tagdefs [TAG_MOVEMENTNUM].tag) == 0) {
        bool    def;
        bool    empty;

        def = strcmp (val [DB_A], "1") == 0;
        empty = strcmp (val [dblocidx], "") == 0;
        if (val [DB_A] != NULL && val [dblocidx] != NULL &&
            ! (def && empty) &&
            strcmp (val [DB_A], val [dblocidx]) != 0) {
          fprintf (stderr, "    tdbcomp:l: tag %s mismatch /%s/%s/ %s\n", tag [DB_A], val [DB_A], val [dblocidx], fn);
          grc = 1;
        }
      } else if (strcmp (tag [DB_A], tagdefs [TAG_DISCNUMBER].tag) == 0) {
        bool    def;
        bool    empty;

        def = strcmp (val [DB_A], "1") == 0;
        empty = strcmp (val [dblocidx], "") == 0;
        if (val [DB_A] != NULL && val [dblocidx] != NULL &&
            ! (def && empty) &&
            strcmp (val [DB_A], val [dblocidx]) != 0) {
          fprintf (stderr, "    tdbcomp:m: tag %s mismatch /%s/%s/ %s\n", tag [DB_A], val [DB_A], val [dblocidx], fn);
          grc = 1;
        }
      } else {
        if (val [DB_A] != NULL && val [dblocidx] != NULL &&
            strcmp (val [DB_A], val [dblocidx]) != 0) {
          fprintf (stderr, "    tdbcomp:n: tag %s mismatch /%s/%s/ %s\n", tag [DB_A], val [DB_A], val [dblocidx], fn);
          grc = 1;
        }
      }
    }

    for (int i = 0; i < DB_MAX; ++i) {
      slistFree (taglist [i]);
    }
  }

  for (int i = 0; i < DB_MAX; ++i) {
    dbClose (db [i]);
  }

  audiosrcCleanup ();
  bdjvarsdfloadCleanup ();
  audiotagCleanup ();
  bdjvarsCleanup ();
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

