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
#include "dirlist.h"
#include "fileop.h"
#include "localeutil.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "slist.h"
#include "song.h"
#include "sysvars.h"
#include "tagdef.h"

#define TEST_MUSIC_DIR    "test-music"

static bool verbose = false;
static bool ignoremissing = false;

static int dbCompare (musicdb_t *db, const char *fn, slist_t *tagdata);

int
main (int argc, char *argv [])
{
  musicdb_t   *db;
  bool        isbdj4 = false;
  const char  *dbfn;
  int         c;
  int         option_index;
  int         grc = 0;
  int         argcount = 0;
  slist_t     *flist;
  slistidx_t  iteridx;
  const char  *fn;
  char        *targ;

  static struct option bdj_options [] = {
    { "bdj4",         no_argument,      NULL,   'B' },
    { "ttagdbchk",    no_argument,      NULL,   0 },
    { "debugself",    no_argument,      NULL,   0 },
    { "wait",         no_argument,      NULL,   0, },
    { "nodetach",     no_argument,      NULL,   0, },
    { "origcwd",      required_argument,  NULL,   0 },
    { "ignoremissing",no_argument,      NULL,   'I', },
    { "verbose",      no_argument,      NULL,   'V', },
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("ttdc");
#endif

  bdj4argInit ();

  while ((c = getopt_long_only (argc, argv, "BV", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      case 'I': {
        ignoremissing = true;
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
    bdj4argCleanup ();
    return 1;
  }

  targ = bdj4argGet (0, argv [0]);
  sysvarsInit (targ);
  bdj4argClear (targ);
  localeInit ();
  bdjoptInit ();
  tagdefInit ();
  audiotagInit ();

  bdjvarsdfloadInit ();

  dbfn = NULL;
  for (int i = optind; i < argc; ++i) {
    if (dbfn == NULL) {
      targ = bdj4argGet (i, argv [i]);
      dbfn = targ;
    }
    argcount++;
  }

  if (argcount < 1) {
    fprintf (stderr, "Usage: ttagdbchk <db-a> (%d)\n", argcount);
    bdj4argCleanup ();
    return 1;
  }
  if (dbfn == NULL || ! fileopFileExists (dbfn)) {
    fprintf (stderr, "no db file %s\n", dbfn);
    bdj4argCleanup ();
    return 1;
  }

  db = dbOpen (dbfn);
  if (db == NULL) {
    fprintf (stderr, "unable to open %s\n", dbfn);
    bdj4argCleanup ();
    return 1;
  }

  /* traverse the directory rather than traversing the database */
  flist = dirlistRecursiveDirList (TEST_MUSIC_DIR, DIRLIST_FILES);
  slistStartIterator (flist, &iteridx);
  while ((fn = slistIterateKey (flist, &iteridx)) != NULL) {
    void    *atdata;
    slist_t *tagdata;
    int     rewrite;

    atdata = audiotagReadTags (fn);
    tagdata = audiotagParseData (fn, atdata, &rewrite);
    if (dbCompare (db, fn, tagdata)) {
      grc = 1;
    }
    dataFree (atdata);
    slistFree (tagdata);
  }

  slistFree (flist);
  dbClose (db);

  bdj4argClear (dbfn);

  audiotagCleanup ();
  bdjvarsdfloadCleanup ();
  tagdefCleanup ();
  bdjoptCleanup ();
  localeCleanup ();
  logEnd ();
  bdj4argCleanup ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return grc;
}

static int
dbCompare (musicdb_t *db, const char *fn, slist_t *tagdata)
{
  int         rc = 0;
  const char  *songnm;
  song_t      *song;

  if (verbose) {
    fprintf (stderr, "-- %s\n", fn);
  }
  songnm = fn + strlen (TEST_MUSIC_DIR) + 1;
  song = dbGetByName (db, songnm);
  if (song == NULL) {
    fprintf (stderr, "  song %s not found\n", songnm);
    return 1;
  }

  if (song != NULL) {
    slist_t     *taglist;
    const char  *tag;
    slistidx_t  tagiteridx;
    slist_t     *processed;

    processed = slistAlloc ("ttagdbchk-proc", LIST_ORDERED, NULL);
    taglist = songTagList (song);

    slistStartIterator (tagdata, &tagiteridx);
    while ((tag = slistIterateKey (tagdata, &tagiteridx)) != NULL) {
      if (verbose) {
        fprintf (stderr, "  audio tag: %s\n", tag);
      }
      slistSetNum (processed, tag, 0);
    }

    slistStartIterator (taglist, &tagiteridx);
    while ((tag = slistIterateKey (taglist, &tagiteridx)) != NULL) {
      const char  *val;
      const char  *tagval;

      slistSetNum (processed, tag, 1);

      if (strcmp (tag, "DBADDDATE") == 0 ||
          strcmp (tag, "FILE") == 0 ||
          strcmp (tag, "LASTUPDATED") == 0 ||
          strcmp (tag, "PFXLEN") == 0 ||
          strcmp (tag, "RRN") == 0 ||
          strcmp (tag, "DURATION") == 0) {
        continue;
      }

      tagval = slistGetStr (tagdata, tag);
      val = slistGetStr (taglist, tag);
      if (verbose) {
        fprintf (stderr, "  db tag: %s %s/%s\n", tag, val, tagval);
      }
      if (val == NULL && tagval != NULL && *tagval) {
        fprintf (stderr, "  missing from db: %s %s\n", tag, tagval);
        rc = 1;
      } else if (ignoremissing == false &&
          val != NULL && *val && tagval == NULL) {
        fprintf (stderr, "  missing audio tag: %s %s\n", tag, val);
        rc = 1;
      } else if (val != NULL && tagval != NULL &&
          strcmp (val, tagval) != 0) {
        fprintf (stderr, "  mismatch: %s %s %s\n", tag, val, tagval);
        rc = 1;
      }
    }

    slistStartIterator (tagdata, &tagiteridx);
    while ((tag = slistIterateKey (tagdata, &tagiteridx)) != NULL) {
      if (strcmp (tag, "ENCODER") == 0) {
        continue;
      }
      if (slistGetNum (processed, tag) == 0) {
        fprintf (stderr, "  missing from db: %s\n", tag);
        rc = 1;
      }
    }

    slistFree (taglist);
    slistFree (processed);
  }

  return rc;
}
