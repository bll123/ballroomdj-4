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
#include "slist.h"
#include "song.h"
#include "songutil.h"
#include "tagdef.h"

typedef struct eibdj4 {
  musicdb_t   *musicdb;
  musicdb_t   *eimusicdb;
  const char  *dirname;
  char        datadir [MAXPATHLEN];
  char        dbfname [MAXPATHLEN];
  char        musicdir [MAXPATHLEN];
  char        origmusicdir [MAXPATHLEN];
  char        *plName;
  char        *newName;
  nlist_t     *dbidxlist;
  nlistidx_t  dbidxiter;
  slistidx_t  dbiteridx;
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
  eibdj4->dbidxlist = NULL;
  eibdj4->plName = NULL;
  eibdj4->newName = NULL;
  eibdj4->eiflag = eiflag;
  eibdj4->counter = 0;
  eibdj4->totcount = 0;
  eibdj4->state = BDJ4_STATE_START;

  strlcpy (eibdj4->origmusicdir,
      bdjoptGetStr (OPT_M_DIR_MUSIC), sizeof (eibdj4->origmusicdir));
  snprintf (eibdj4->musicdir, sizeof (eibdj4->musicdir),
      "%s/music", eibdj4->dirname);
  snprintf (eibdj4->datadir, sizeof (eibdj4->datadir),
      "%s/data", eibdj4->dirname);
  snprintf (eibdj4->dbfname, sizeof (eibdj4->dbfname),
      "%s/%s%s", eibdj4->datadir, MUSICDB_FNAME, MUSICDB_EXT);

  return eibdj4;
}

void
eibdj4Free (eibdj4_t *eibdj4)
{
  if (eibdj4 != NULL) {
    dataFree (eibdj4->plName);
    dataFree (eibdj4->newName);
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
eibdj4SetPlaylist (eibdj4_t *eibdj4, const char *name)
{
  if (eibdj4 == NULL) {
    return;
  }
  if (name == NULL) {
    return;
  }

  eibdj4->plName = mdstrdup (name);
}

void
eibdj4SetNewName (eibdj4_t *eibdj4, const char *name)
{
  if (eibdj4 == NULL) {
    return;
  }
  if (name == NULL) {
    return;
  }

  eibdj4->newName = mdstrdup (name);
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
    if (eibdj4->plName == NULL) {
      eibdj4->state = BDJ4_STATE_OFF;
      return rc;
    }
    if (eibdj4->newName == NULL) {
      eibdj4->state = BDJ4_STATE_OFF;
      return rc;
    }
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
    eibdj4->totcount = nlistGetCount (eibdj4->dbidxlist);

    diropMakeDir (eibdj4->datadir);
    diropMakeDir (eibdj4->musicdir);

    pathbldMakePath (from, sizeof (from),
        eibdj4->plName, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
    snprintf (tbuff, sizeof (tbuff), "%s/%s%s", eibdj4->datadir,
        eibdj4->plName, BDJ4_PLAYLIST_EXT);
    filemanipCopy (from, tbuff);

    pathbldMakePath (from, sizeof (from),
        eibdj4->plName, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DREL_DATA);
    snprintf (tbuff, sizeof (tbuff), "%s/%s%s", eibdj4->datadir,
        eibdj4->plName, BDJ4_PL_DANCE_EXT);
    filemanipCopy (from, tbuff);

    pathbldMakePath (from, sizeof (from),
        eibdj4->plName, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
    snprintf (tbuff, sizeof (tbuff), "%s/%s%s", eibdj4->datadir,
        eibdj4->plName, BDJ4_SONGLIST_EXT);
    filemanipCopy (from, tbuff);

    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->musicdir);
    eibdj4->eimusicdb = dbOpen (eibdj4->dbfname);
    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->origmusicdir);
    dbStartBatch (eibdj4->eimusicdb);
    dbDisableLastUpdateTime (eibdj4->eimusicdb);

    eibdj4->state = BDJ4_STATE_PROCESS;
    rc = false;
  }

  if (eibdj4->state == BDJ4_STATE_PROCESS) {
    song_t      *song;
    song_t      *tsong;
    bool        doupdate = false;
    bool        docopy = false;
    const char  *songfn;
    char        *ffn = NULL;
    char        tfn [MAXPATHLEN];
    bool        isabsolute = false;
    pathinfo_t  *pi;

    if ((dbidx = nlistIterateKey (eibdj4->dbidxlist, &eibdj4->dbidxiter)) >= 0) {
      song = dbGetByIdx (eibdj4->musicdb, dbidx);

      songfn = songGetStr (song, TAG_FILE);
      strlcpy (tfn, songfn, sizeof (tfn));
      ffn = songutilFullFileName (songfn);
      if (songutilIsAbsolutePath (songfn)) {
        isabsolute = true;
        pi = pathInfo (songfn);
        snprintf (tfn, sizeof (tfn), "%.*s", (int) pi->flen, pi->filename);
        pathInfoFree (pi);
      }
      /* tbuff holds new full pathname of the exported song */
      snprintf (tbuff, sizeof (tbuff), "%s/%s", eibdj4->musicdir, tfn);
      if (isabsolute) {
        songSetStr (song, TAG_FILE, tfn);
      }

      tsong = dbGetByName (eibdj4->eimusicdb, tfn);
      if (tsong == NULL) {
        songSetNum (song, TAG_RRN, MUSICDB_ENTRY_NEW);
        doupdate = true;
        docopy = true;
      } else {
        time_t    oupd;
        time_t    nupd;

        oupd = songGetNum (tsong, TAG_LAST_UPDATED);
        nupd = songGetNum (song, TAG_LAST_UPDATED);
        if (nupd > oupd) {
          doupdate = true;
          if (bdjoptGetNum (OPT_G_WRITETAGS) != WRITE_TAGS_NONE) {
            docopy = true;
          }
          songSetNum (song, TAG_RRN, songGetNum (tsong, TAG_RRN));
        }
        if (! docopy && ! fileopFileExists (tbuff)) {
          docopy = true;
        }
      }

      if (doupdate) {
        pathinfo_t  *pi;

        pi = pathInfo (tbuff);
        snprintf (tfn, sizeof (tfn), "%.*s", (int) pi->dlen, pi->dirname);
        diropMakeDir (tfn);
        pathInfoFree (pi);

        dbWriteSong (eibdj4->eimusicdb, song);
      }
      if (docopy) {
        filemanipCopy (ffn, tbuff);
      }
      eibdj4->counter += 1;
      rc = false;
    } else {
      eibdj4->state = BDJ4_STATE_FINISH;
    }

    dataFree (ffn);
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
  bool        rc = true;

  if (eibdj4 == NULL) {
    return rc;
  }

  if (eibdj4->state == BDJ4_STATE_START) {
    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->musicdir);
    eibdj4->eimusicdb = dbOpen (eibdj4->dbfname);
    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->origmusicdir);
    eibdj4->totcount = dbCount (eibdj4->eimusicdb);

    dbDisableLastUpdateTime (eibdj4->musicdb);
    dbStartIterator (eibdj4->eimusicdb, &eibdj4->dbiteridx);

    eibdj4->state = BDJ4_STATE_PROCESS;
    rc = false;
  }

  if (eibdj4->state == BDJ4_STATE_PROCESS) {
    song_t    *song;
    dbidx_t   dbidx;

    song = dbIterate (eibdj4->eimusicdb, &dbidx, &eibdj4->dbiteridx);

    if (song != NULL) {
      const char    *songfn;
      char          *ffn = NULL;
      char          *nfn = NULL;
      char          tbuff [MAXPATHLEN];
      song_t        *tsong;

      songfn = songGetStr (song, TAG_FILE);
      snprintf (tbuff, sizeof (tbuff), "%s/%s", eibdj4->musicdir, songfn);

      tsong = dbGetByName (eibdj4->musicdb, songfn);

      /* only import an audio file if it does not exist in the database */
      if (tsong == NULL) {
fprintf (stderr, "%s is new\n", songfn);
        dbWriteSong (eibdj4->musicdb, song);
        nfn = songutilFullFileName (songfn);
        filemanipCopy (tbuff, ffn);
      } else {
fprintf (stderr, "found %s in main db\n", songfn);
      }

      dataFree (ffn);
      dataFree (nfn);
      eibdj4->counter += 1;
      rc = false;
    } else {
      eibdj4->state = BDJ4_STATE_FINISH;
    }
  }

  if (eibdj4->state == BDJ4_STATE_FINISH) {
    dbEnableLastUpdateTime (eibdj4->musicdb);
    dbClose (eibdj4->eimusicdb);
    rc = true;
  }

  return rc;
}

