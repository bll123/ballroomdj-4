#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "bdjopt.h"
#include "dirop.h"
#include "expimpbdj4.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathutil.h"
#include "pathbld.h"
#include "song.h"
#include "songutil.h"
#include "tagdef.h"

typedef struct eibdj4 {
  musicdb_t   *musicdb;
  musicdb_t   *eimusicdb;
  const char  *dirname;
  char        musicdir [MAXPATHLEN];
  char        origmusicdir [MAXPATHLEN];
  nlist_t     *dbidxlist;
  nlistidx_t  dbidxiter;
  char        *plName;
  const char  *newName;
  int         eiflag;
  int         counter;
  int         totcount;
  int         state;
} eibdj4_t;

static bool eibdj4ProcessExport (eibdj4_t *eibdj4);
static bool eibdj4ProcessImport (eibdj4_t *eibdj4);

eibdj4_t *
eibdj4Init (musicdb_t *musicdb, const char *dirname, int eiflag)
{
  eibdj4_t  *eibdj4;

  eibdj4 = mdmalloc (sizeof (eibdj4_t));
  eibdj4->musicdb = musicdb;
  eibdj4->dirname = dirname;
  strlcpy (eibdj4->origmusicdir,
      bdjoptGetStr (OPT_M_DIR_MUSIC), sizeof (eibdj4->origmusicdir));
  snprintf (eibdj4->musicdir, sizeof (eibdj4->musicdir),
      "%s/music", eibdj4->dirname);
  eibdj4->dbidxlist = NULL;
  eibdj4->plName = NULL;
  eibdj4->newName = NULL;
  eibdj4->eiflag = eiflag;
  eibdj4->counter = 0;
  eibdj4->totcount = 0;
  eibdj4->state = BDJ4_STATE_START;

  return eibdj4;
}

void
eibdj4Free (eibdj4_t *eibdj4)
{
  if (eibdj4 != NULL) {
    dataFree (eibdj4->plName);
    mdfree (eibdj4);
  }
}

void
eibdj4SetDBIdxList (eibdj4_t *eibdj4, nlist_t *dbidxlist)
{
  if (eibdj4 == NULL) {
    return;
  }

  eibdj4->dbidxlist = dbidxlist;
}

void
eibdj4SetName (eibdj4_t *eibdj4, const char *name)
{
  if (eibdj4 == NULL) {
    return;
  }

  eibdj4->plName = mdstrdup (name);
}

void
eibdj4GetCount (eibdj4_t *eibdj4, int *count, int *tot)
{
  if (eibdj4 == NULL) {
    *count = 0;
    *tot = 0;
    return;
  }

  *count = eibdj4->counter;
  *tot = eibdj4->totcount;
}

bool
eibdj4Process (eibdj4_t *eibdj4)
{
  bool    rc = true;

  if (eibdj4 == NULL) {
    return rc;
  }

  if (eibdj4->state == BDJ4_STATE_OFF ||
      eibdj4->state == BDJ4_STATE_FINISH) {
    return rc;
  }

  if (eibdj4->eiflag == EIBDJ4_EXPORT) {
    if (eibdj4->dbidxlist == NULL) {
      eibdj4->state = BDJ4_STATE_OFF;
      return rc;
    }
    if (eibdj4->plName == NULL) {
      eibdj4->state = BDJ4_STATE_OFF;
      return rc;
    }
    rc = eibdj4ProcessExport (eibdj4);
  }
  if (eibdj4->eiflag == EIBDJ4_IMPORT) {
    rc = eibdj4ProcessImport (eibdj4);
  }

  return rc;
}

/* internal routines */

static bool
eibdj4ProcessExport (eibdj4_t *eibdj4)
{
  bool      rc = true;
  dbidx_t   dbidx;
  char      tbuff [MAXPATHLEN];
  char      from [MAXPATHLEN];

  if (eibdj4 == NULL) {
    return rc;
  }

  if (eibdj4->state == BDJ4_STATE_START) {
    nlistStartIterator (eibdj4->dbidxlist, &eibdj4->dbidxiter);

    snprintf (tbuff, sizeof (tbuff), "%s/data", eibdj4->dirname);
    diropMakeDir (tbuff);
    diropMakeDir (eibdj4->musicdir);

    pathbldMakePath (from, sizeof (from),
        eibdj4->plName, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
    snprintf (tbuff, sizeof (tbuff), "%s/data/%s%s", eibdj4->dirname,
        eibdj4->plName, BDJ4_PLAYLIST_EXT);
    filemanipCopy (from, tbuff);

    pathbldMakePath (from, sizeof (from),
        eibdj4->plName, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DREL_DATA);
    snprintf (tbuff, sizeof (tbuff), "%s/data/%s%s", eibdj4->dirname,
        eibdj4->plName, BDJ4_PL_DANCE_EXT);
    filemanipCopy (from, tbuff);

    pathbldMakePath (from, sizeof (from),
        eibdj4->plName, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
    snprintf (tbuff, sizeof (tbuff), "%s/data/%s%s", eibdj4->dirname,
        eibdj4->plName, BDJ4_SONGLIST_EXT);
    filemanipCopy (from, tbuff);

    snprintf (tbuff, sizeof (tbuff), "%s/data/%s%s", eibdj4->dirname,
        MUSICDB_FNAME, MUSICDB_EXT);
    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->musicdir);
    eibdj4->eimusicdb = dbOpen (tbuff);
    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->origmusicdir);
    dbStartBatch (eibdj4->eimusicdb);
    dbDisableLastUpdateTime (eibdj4->eimusicdb);

    eibdj4->state = BDJ4_STATE_PROCESS;
    rc = false;
  }

  if (eibdj4->state == BDJ4_STATE_PROCESS) {
    song_t      *song;
    song_t      *tsong;
    bool        doupdate = true;
    const char  *fstr;
    char        *ffn;
    char        tfn [MAXPATHLEN];
    bool        isabsolute = false;
    pathinfo_t  *pi;

    if ((dbidx = nlistIterateKey (eibdj4->dbidxlist, &eibdj4->dbidxiter)) >= 0) {
fprintf (stderr, "export %d\n", dbidx);
      song = dbGetByIdx (eibdj4->musicdb, dbidx);

      fstr = songGetStr (song, TAG_FILE);
fprintf (stderr, "  fn: %s\n", fstr);
      strlcpy (tfn, fstr, sizeof (tfn));
      ffn = songutilFullFileName (fstr);
      if (songutilIsAbsolutePath (fstr)) {
        isabsolute = true;
        pi = pathInfo (fstr);
        snprintf (tfn, sizeof (tfn), "%.*s", (int) pi->flen, pi->filename);
        pathInfoFree (pi);
      }
      /* tbuff holds new full pathname of the exported song */
      snprintf (tbuff, sizeof (tbuff), "%s/%s", eibdj4->musicdir, tfn);
fprintf (stderr, "  path: %s\n", tbuff);
      if (isabsolute) {
        songSetStr (song, TAG_FILE, tfn);
      }

fprintf (stderr, "  search: %s\n", tfn);
      tsong = dbGetByName (eibdj4->eimusicdb, tfn);
      if (tsong == NULL) {
fprintf (stderr, "  song not found: new\n");
        songSetNum (song, TAG_RRN, MUSICDB_ENTRY_NEW);
      } else {
        time_t    oupd;
        time_t    nupd;

        oupd = songGetNum (tsong, TAG_LAST_UPDATED);
        nupd = songGetNum (song, TAG_LAST_UPDATED);
        doupdate = false;
fprintf (stderr, "  song found: %ld > %ld?\n", nupd, oupd);
        if (nupd > oupd) {
          doupdate = true;
          songSetNum (song, TAG_RRN, songGetNum (tsong, TAG_RRN));
        }
      }

      if (doupdate) {
        pathinfo_t  *pi;

fprintf (stderr, "  doupdate\n");
        pi = pathInfo (tbuff);
        snprintf (tfn, sizeof (tfn), "%.*s", (int) pi->dlen, pi->dirname);
fprintf (stderr, "  mkdir: %s\n", tfn);
        diropMakeDir (tfn);
        pathInfoFree (pi);

        dbWriteSong (eibdj4->eimusicdb, song);
      }
      if (doupdate || ! fileopFileExists (tbuff)) {
fprintf (stderr, "copy-ffn: %s\n", ffn);
fprintf (stderr, "     new: %s\n", tbuff);
        filemanipCopy (ffn, tbuff);
      }
      rc = false;
    } else {
      eibdj4->state = BDJ4_STATE_FINISH;
    }
  }

  if (eibdj4->state == BDJ4_STATE_FINISH) {
    dbEndBatch (eibdj4->eimusicdb);
    dbClose (eibdj4->eimusicdb);
    eibdj4->dbidxlist = NULL;
    rc = true;
  }

  return rc;
}

static bool
eibdj4ProcessImport (eibdj4_t *eibdj4)
{
  bool  rc = true;
  char  tbuff [MAXPATHLEN];

  if (eibdj4 == NULL) {
    return rc;
  }

  if (eibdj4->state == BDJ4_STATE_START) {
    snprintf (tbuff, sizeof (tbuff), "%s/data/%s%s", eibdj4->dirname,
        MUSICDB_FNAME, MUSICDB_EXT);
    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->musicdir);
    eibdj4->eimusicdb = dbOpen (tbuff);
    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->origmusicdir);
    eibdj4->state = BDJ4_STATE_PROCESS;
    rc = false;
  }

  if (eibdj4->state == BDJ4_STATE_PROCESS) {
    eibdj4->state = BDJ4_STATE_FINISH;
    rc = false;
  }

  if (eibdj4->state == BDJ4_STATE_FINISH) {
    dbClose (eibdj4->eimusicdb);
    rc = true;
  }

  return rc;
}

