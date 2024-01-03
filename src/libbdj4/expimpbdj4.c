/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "bdjopt.h"
#include "dirop.h"
#include "expimpbdj4.h"
#include "fileop.h"
#include "filemanip.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathinfo.h"
#include "pathbld.h"
#include "slist.h"
#include "song.h"
#include "songdb.h"
#include "songlist.h"
#include "tagdef.h"

typedef struct eibdj4 {
  musicdb_t   *musicdb;
  songdb_t    *songdb;
  musicdb_t   *eimusicdb;
  const char  *dirname;
  char        datadir [MAXPATHLEN];
  char        dbfname [MAXPATHLEN];
  char        musicdir [MAXPATHLEN];
  char        origmusicdir [MAXPATHLEN];
  char        *plName;
  char        *newName;
  /* export uses the dbidxlist to determine which songs to export */
  nlist_t     *dbidxlist;
  nlistidx_t  dbidxiter;
  slistidx_t  dbiteridx;
  songlist_t  *sl;
  ilistidx_t  slkey;
  ilistidx_t  sliteridx;
  int         eiflag;
  int         counter;
  int         totcount;
  int         state;
  bool        dbchanged : 1;
} eibdj4_t;

static bool eibdj4ProcessExport (eibdj4_t *eibdj4);
static bool eibdj4ProcessImport (eibdj4_t *eibdj4);

eibdj4_t *
eibdj4Init (musicdb_t *musicdb, const char *dirname, int eiflag)
{
  eibdj4_t  *eibdj4;

  eibdj4 = mdmalloc (sizeof (eibdj4_t));
  eibdj4->musicdb = musicdb;
  eibdj4->songdb = songdbAlloc (musicdb);
  eibdj4->dirname = dirname;
  eibdj4->dbidxlist = NULL;
  eibdj4->plName = NULL;
  eibdj4->newName = NULL;
  eibdj4->sl = NULL;
  eibdj4->eiflag = eiflag;
  eibdj4->counter = 0;
  eibdj4->totcount = 0;
  eibdj4->state = BDJ4_STATE_START;
  eibdj4->dbchanged = false;

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
    songdbFree (eibdj4->songdb);
    dataFree (eibdj4->plName);
    dataFree (eibdj4->newName);
    songlistFree (eibdj4->sl);
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

bool
eibdj4DatabaseChanged (eibdj4_t *eibdj4)
{
  if (eibdj4 == NULL) {
    return false;
  }

  return eibdj4->dbchanged;
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

    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->musicdir);
    eibdj4->eimusicdb = dbOpen (eibdj4->dbfname);
    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->origmusicdir);
    dbStartBatch (eibdj4->eimusicdb);
    dbDisableLastUpdateTime (eibdj4->eimusicdb);

    snprintf (tbuff, sizeof (tbuff), "%s/%s%s", eibdj4->datadir,
        eibdj4->plName, BDJ4_SONGLIST_EXT);
    songlistFree (eibdj4->sl);
    eibdj4->sl = songlistCreate (tbuff);
    eibdj4->slkey = 0;

    eibdj4->state = BDJ4_STATE_PROCESS;
    rc = false;
  }

  if (eibdj4->state == BDJ4_STATE_PROCESS) {
    song_t      *song;
    song_t      *tsong;
    bool        doupdate = false;
    bool        docopy = false;
    const char  *songfn;
    char        ffn [MAXPATHLEN];
    char        nsongfn [MAXPATHLEN];
    bool        isabsolute = false;
    int         type;

    if ((dbidx = nlistIterateKey (eibdj4->dbidxlist, &eibdj4->dbidxiter)) >= 0) {
      song = dbGetByIdx (eibdj4->musicdb, dbidx);

      if (song == NULL) {
        return false;
      }

      songfn = songGetStr (song, TAG_URI);
      type = audiosrcGetType (songfn);

      strlcpy (nsongfn, songfn, sizeof (nsongfn));
      audiosrcFullPath (songfn, ffn, sizeof (ffn), 0, NULL);

      if (type == AUDIOSRC_TYPE_FILE &&
          fileopIsAbsolutePath (songfn)) {
        int     pfxlen;

        isabsolute = true;
        pfxlen = songGetNum (song, TAG_PREFIX_LEN);
        if (pfxlen < 0) {
          pfxlen = 0;
        }
        snprintf (nsongfn, sizeof (nsongfn), "%s", songfn + pfxlen);
      }

      /* tbuff holds new full pathname of the exported song */
      snprintf (tbuff, sizeof (tbuff), "%s/%s", eibdj4->musicdir, nsongfn);
      if (isabsolute) {
        songSetStr (song, TAG_URI, nsongfn);
      }

      doupdate = false;
      docopy = false;

      /* a song in a secondary folder w/the same name is going to be */
      /* kicked out, as it will be found in the export music-db */
      tsong = dbGetByName (eibdj4->eimusicdb, nsongfn);
      if (tsong == NULL) {
        songSetNum (song, TAG_RRN, MUSICDB_ENTRY_NEW);
        doupdate = true;
        if (type == AUDIOSRC_TYPE_FILE) {
          docopy = true;
        }
      } else {
        time_t    oupd;
        time_t    nupd;

        oupd = songGetNum (tsong, TAG_LAST_UPDATED);
        nupd = songGetNum (song, TAG_LAST_UPDATED);
        if (nupd > oupd) {
          doupdate = true;
          songSetNum (song, TAG_RRN, songGetNum (tsong, TAG_RRN));
        }
        if (type == AUDIOSRC_TYPE_FILE &&
            ! docopy && ! fileopFileExists (tbuff)) {
          docopy = true;
        }
      }

      songlistSetStr (eibdj4->sl, eibdj4->slkey, SONGLIST_URI, nsongfn);
      songlistSetStr (eibdj4->sl, eibdj4->slkey, SONGLIST_TITLE,
          songGetStr (song, TAG_TITLE));
      songlistSetNum (eibdj4->sl, eibdj4->slkey, SONGLIST_DANCE,
          songGetNum (song, TAG_DANCE));
      ++eibdj4->slkey;

      if (doupdate) {
        pathinfo_t  *pi;
        char        tdir [MAXPATHLEN];

        if (type == AUDIOSRC_TYPE_FILE) {
          pi = pathInfo (tbuff);
          pathInfoGetDir (pi, tdir, sizeof (tdir));
          diropMakeDir (tdir);
          pathInfoFree (pi);
        }

        dbWriteSong (eibdj4->eimusicdb, song);
      }

      if (type == AUDIOSRC_TYPE_FILE && docopy) {
        filemanipCopy (ffn, tbuff);
      }
      eibdj4->counter += 1;
      rc = false;
    } else {
      eibdj4->state = BDJ4_STATE_FINISH;
    }
  }

  if (eibdj4->state == BDJ4_STATE_FINISH) {
    songlistSave (eibdj4->sl, SONGLIST_UPDATE_TIMESTAMP, 1);
    songlistFree (eibdj4->sl);
    eibdj4->sl = NULL;
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
    char        tbuff [MAXPATHLEN];

    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->musicdir);
    eibdj4->eimusicdb = dbOpen (eibdj4->dbfname);
    bdjoptSetStr (OPT_M_DIR_MUSIC, eibdj4->origmusicdir);

    dbDisableLastUpdateTime (eibdj4->musicdb);

    snprintf (tbuff, sizeof (tbuff), "%s/%s%s", eibdj4->datadir,
        eibdj4->plName, BDJ4_SONGLIST_EXT);
    eibdj4->sl = songlistLoad (tbuff);
    songlistStartIterator (eibdj4->sl, &eibdj4->sliteridx);
    eibdj4->totcount = songlistGetCount (eibdj4->sl);

    eibdj4->state = BDJ4_STATE_PROCESS;
    rc = false;
  }

  if (eibdj4->state == BDJ4_STATE_PROCESS) {
    song_t      *song;
    ilistidx_t  slidx;
    const char  *slfn;

    slidx = songlistIterate (eibdj4->sl, &eibdj4->sliteridx);
    if (slidx >= 0) {
      const char    *songfn;
      char          nfn [MAXPATHLEN];
      char          ifn [MAXPATHLEN];
      song_t        *tsong;
      bool          doupdate;
      bool          docopy;
      int           type;

      slfn = songlistGetStr (eibdj4->sl, slidx, SONGLIST_URI);
      song = dbGetByName (eibdj4->eimusicdb, slfn);

      if (song == NULL) {
        return false;
      }

      songfn = songGetStr (song, TAG_URI);
      type = audiosrcGetType (songfn);
      audiosrcFullPath (songfn, nfn, sizeof (nfn), 0, NULL);
      snprintf (ifn, sizeof (ifn), "%s/%s", eibdj4->musicdir, songfn);

      tsong = dbGetByName (eibdj4->musicdb, songfn);

      doupdate = false;
      docopy = false;

      /* only import an audio file if it does not exist in the database. */
      if (tsong == NULL) {
        doupdate = true;
        if (type == AUDIOSRC_TYPE_FILE) {
          docopy = true;
        }
      }
      if (tsong != NULL) {
        time_t    oupd;
        time_t    nupd;

        oupd = songGetNum (tsong, TAG_LAST_UPDATED);
        nupd = songGetNum (song, TAG_LAST_UPDATED);
        if (nupd > oupd) {
          doupdate = true;
        }
      }
      if (type == AUDIOSRC_TYPE_FILE &&
          ! docopy && ! fileopFileExists (nfn)) {
        docopy = true;
      }
      if (doupdate) {
        int     songdbflags;

        eibdj4->dbchanged = true;
        songSetChanged (song);
        songdbflags = SONGDB_NONE;
        songdbWriteDBSong (eibdj4->songdb, song,
            &songdbflags, songGetNum (song, TAG_RRN));
      }
      if (docopy) {
        pathinfo_t  *pi;
        char        tdir [MAXPATHLEN];

        pi = pathInfo (nfn);
        pathInfoGetDir (pi, tdir, sizeof (tdir));
        diropMakeDir (tdir);
        filemanipCopy (ifn, nfn);
        pathInfoFree (pi);
      }

      eibdj4->counter += 1;
      rc = false;
    } else {
      char  from [MAXPATHLEN];
      char  to [MAXPATHLEN];

      /* only copy the song list after the database has been updated */

      snprintf (from, sizeof (from), "%s/%s%s", eibdj4->datadir,
          eibdj4->plName, BDJ4_PLAYLIST_EXT);
      pathbldMakePath (to, sizeof (to),
          eibdj4->newName, BDJ4_PLAYLIST_EXT, PATHBLD_MP_DREL_DATA);
      filemanipCopy (from, to);

      snprintf (from, sizeof (from), "%s/%s%s", eibdj4->datadir,
          eibdj4->plName, BDJ4_PL_DANCE_EXT);
      pathbldMakePath (to, sizeof (to),
          eibdj4->newName, BDJ4_PL_DANCE_EXT, PATHBLD_MP_DREL_DATA);
      filemanipCopy (from, to);

      snprintf (from, sizeof (from), "%s/%s%s", eibdj4->datadir,
          eibdj4->plName, BDJ4_SONGLIST_EXT);
      pathbldMakePath (to, sizeof (to),
          eibdj4->newName, BDJ4_SONGLIST_EXT, PATHBLD_MP_DREL_DATA);
      filemanipCopy (from, to);

      eibdj4->state = BDJ4_STATE_FINISH;
    }
  }

  if (eibdj4->state == BDJ4_STATE_FINISH) {
    songlistFree (eibdj4->sl);
    eibdj4->sl = NULL;
    dbEnableLastUpdateTime (eibdj4->musicdb);
    dbClose (eibdj4->eimusicdb);
    rc = true;
  }

  return rc;
}

