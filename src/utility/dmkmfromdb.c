/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4arg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdf.h"
#include "bdjvarsdfload.h"
#include "dance.h"
#include "dirop.h"
#include "filemanip.h"
#include "fileop.h"
#include "localeutil.h"
#include "log.h"
#include "ilist.h"
#include "mdebug.h"
#include "musicdb.h"
#include "osdirutil.h"
#include "pathinfo.h"
#include "rafile.h"
#include "slist.h"
#include "song.h"
#include "sysvars.h"
#include "tagdef.h"

static char *tmusicorig = "test-music-orig";
static char *tmusicdir = "test-music";

static song_t * dbReadEntry (rafile_t *radb, rafileidx_t rrn);
static void stripSpaces (char *str);

int
main (int argc, char *argv [])
{
  int         c = 0;
  int         option_index = 0;
  bool        isbdj4 = false;
  char        dbfn [MAXPATHLEN];
  char        cwd [MAXPATHLEN];
  char        tofdir [MAXPATHLEN];
  char        tmdir [MAXPATHLEN];
  loglevel_t  loglevel = LOG_IMPORTANT | LOG_INFO | LOG_RAFILE;
  bool        loglevelset = false;
  rafile_t    *radb;
  pathinfo_t  *pi;
  dance_t     *dances;
  bdj4arg_t   *bdj4arg;
  const char  *targ;

  static struct option bdj_options [] = {
    { "dmkmfromdb",   no_argument,        NULL,   0 },
    { "bdj4",         no_argument,        NULL,   'B' },
    /* launcher options */
    { "debugself",    no_argument,        NULL,   0 },
    { "debug",        required_argument,  NULL,   'd' },
    { "nodetach",     no_argument,        NULL,   0, },
    { "origcwd",      required_argument,  NULL,   0 },
    { "scale",        required_argument,  NULL,   0 },
    { "theme",        required_argument,  NULL,   0 },
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("dmfd");
#endif

  bdj4arg = bdj4argInit (argc, argv);

  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "Bd:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
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
    bdj4argCleanup (bdj4arg);
    return 1;
  }

  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ, SYSVARS_FLAG_ALL);
  localeInit ();
  bdjoptInit ();
  bdjvarsInit ();
  tagdefInit ();

  osGetCurrentDir (cwd, sizeof (cwd));
  stpecpy (dbfn, dbfn + sizeof (dbfn), "data/musicdb.dat");
  snprintf (tofdir, sizeof (tofdir), "%s/%s",
      sysvarsGetStr (SV_BDJ4_DIR_MAIN), tmusicorig);
  snprintf (tmdir, sizeof (tmdir), "%s/%s",
      sysvarsGetStr (SV_BDJ4_DIR_MAIN), tmusicdir);
  bdjoptSetStr (OPT_M_DIR_MUSIC, tmdir);

  bdjvarsdfloadInit ();
  audiosrcInit ();

  if (! loglevelset) {
    loglevel = bdjoptGetNum (OPT_G_DEBUGLVL);
  }
  logStartAppend ("dmkmfromdb", "dmfd", loglevel);

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  radb = raOpen (dbfn, MUSICDB_VERSION, RAFILE_RW);

  raStartBatch (radb);

  for (rafileidx_t i = 1; i <= raGetCount (radb); ++i) {
    song_t    *song;

    song = dbReadEntry (radb, i);

    if (song != NULL) {
      const char  *fstr;
      ilistidx_t  dkey;
      char        ext [40];
      char        tdnc [100];
      char        ofn [MAXPATHLEN];
      char        nfn [MAXPATHLEN];
      char        tdir [MAXPATHLEN];

      dkey = songGetNum (song, TAG_DANCE);
      if (dkey < 0) {
        /* unknown dance, just use waltz music */
        stpecpy (tdnc, tdnc + sizeof (tdnc), "waltz");
      } else {
        stpecpy (tdnc, tdnc + sizeof (tdnc), danceGetStr (dances, dkey, DANCE_DANCE));
        stripSpaces (tdnc);
        stringAsciiToLower (tdnc);
      }
      fstr = songGetStr (song, TAG_URI);

      pi = pathInfo (fstr);
      pathInfoGetExt (pi, ext, sizeof (ext));
      snprintf (ofn, sizeof (ofn), "%s/%s%s", tofdir, tdnc, ext);
      if (! fileopFileExists (ofn)) {
        snprintf (ofn, sizeof (ofn), "%s/%s%s", tofdir, "waltz", ext);
        if (! fileopFileExists (ofn)) {
          continue;
        }
      }
      pathInfoGetDir (pi, tdir, sizeof (tdir));
      snprintf (nfn, sizeof (nfn), "%s/%s", tmdir, tdir);
      diropMakeDir (nfn);
      snprintf (nfn, sizeof (nfn), "%s/%s", tmdir, fstr);
      filemanipCopy (ofn, nfn);
    }
  }

  raEndBatch (radb);
  raClose (radb);

  audiosrcCleanup ();
  bdjvarsdfloadCleanup ();
  tagdefCleanup ();
  bdjvarsCleanup ();
  bdjoptCleanup ();
  localeCleanup ();
  logEnd ();
  bdj4argCleanup (bdj4arg);
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

static song_t *
dbReadEntry (rafile_t *radb, rafileidx_t rrn)
{
  int     rc;
  song_t  *song;
  char    data [RAFILE_REC_SIZE];

  *data = '\0';
  rc = raRead (radb, rrn, data);
  if (rc != 1) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: Unable to access rrn %" PRId32, rrn);
  }
  if (rc == 0 || ! *data) {
    return NULL;
  }

  song = songAlloc ();
  songParse (song, data, rrn);
  return song;
}

static void
stripSpaces (char *str)
{
  char    *optr = str;
  char    *nptr = str;

  while (*optr) {
    while (*optr == ' ') {
      ++optr;
    }
    *nptr = *optr;
    ++optr;
    ++nptr;
  }
  *nptr = '\0';
}
