/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "bdj4.h"
#include "audiofile.h"
#include "audiosrc.h"
#include "audiotag.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "dirop.h"
#include "fileop.h"
#include "filemanip.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "orgutil.h"
#include "pathinfo.h"
#include "playlist.h"
#include "slist.h"
#include "song.h"
#include "songdb.h"
#include "songlist.h"
#include "tagdef.h"

enum {
  SONGDB_IDENT = 0xaa006264676e6f73,
};

typedef struct songdb {
  uint64_t  ident;
  musicdb_t *musicdb;
  org_t     *org;
  org_t     *orgold;
} songdb_t;

static bool songdbNewName (songdb_t *songdb, song_t *song, char *newuri, size_t sz);
static void songdbWriteAudioTags (song_t *song, int forceflag);
static void songdbUpdateAllSonglists (song_t *song, const char *olduri);

songdb_t *
songdbAlloc (musicdb_t *musicdb)
{
  songdb_t *songdb;
  const char  *torgpath;

  songdb = mdmalloc (sizeof (songdb_t));
  songdb->ident = SONGDB_IDENT;
  songdb->musicdb = musicdb;

  torgpath = bdjoptGetStr (OPT_G_ORGPATH);
  songdb->org = orgAlloc (torgpath);
  songdb->orgold = NULL;
  if (strstr (torgpath, "BYPASS") != NULL) {
    songdb->orgold = orgAlloc (bdjoptGetStr (OPT_G_OLDORGPATH));
  }

  return songdb;
}

void
songdbFree (songdb_t *songdb)
{
  if (songdb == NULL || songdb->ident != SONGDB_IDENT) {
    return;
  }

  songdb->ident = BDJ4_IDENT_FREE;
  orgFree (songdb->org);
  songdb->org = NULL;
  orgFree (songdb->orgold);
  mdfree (songdb);
}

void
songdbSetMusicDB (songdb_t *songdb, musicdb_t *musicdb)
{
  if (songdb == NULL || songdb->ident != SONGDB_IDENT) {
    return;
  }

  songdb->musicdb = musicdb;
}

void
songdbWriteDB (songdb_t *songdb, dbidx_t dbidx, int forceflag)
{
  song_t    *song;
  int32_t   songdbflags;

  if (songdb == NULL || songdb->ident != SONGDB_IDENT ||
      songdb->musicdb == NULL) {
    return;
  }
  song = dbGetByIdx (songdb->musicdb, dbidx);
  songdbflags = SONGDB_NONE;
  if (forceflag) {
    songdbflags |= SONGDB_FORCE_WRITE;
  }
  songdbWriteDBSong (songdb, song, &songdbflags, songGetNum (song, TAG_RRN));
}

/* flags is both input and output */
int32_t
songdbWriteDBSong (songdb_t *songdb, song_t *song, int *flags, dbidx_t rrn)
{
  char        newuri [MAXPATHLEN];
  char        newffn [MAXPATHLEN];
  char        olduri [MAXPATHLEN];
  char        ffn [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  char        dirbuff [MAXPATHLEN];
  bool        dorename = false;
  bool        renameallow = false;
  bool        renamesuccess = false;
  pathinfo_t  *pi;
  int         rc;

  if (songdb == NULL || songdb->ident != SONGDB_IDENT ||
      songdb->musicdb == NULL) {
    *flags |= SONGDB_RET_NULL;
    return 0;
  }
  if (song == NULL) {
    *flags |= SONGDB_RET_NULL;
    return 0;
  }
  if ((*flags & SONGDB_FORCE_WRITE) != SONGDB_FORCE_WRITE &&
      ! songIsChanged (song)) {
    *flags |= SONGDB_RET_NO_CHANGE;
    return 0;
  }
  /* do not write temporary or removed songs */
  if (songGetNum (song, TAG_DB_FLAGS) != MUSICDB_STD) {
    *flags |= SONGDB_RET_TEMP;
    return 0;
  }

  renameallow = (bdjoptGetNum (OPT_G_AUTOORGANIZE) == true) ||
      ((*flags & SONGDB_FORCE_RENAME) == SONGDB_FORCE_RENAME);

  if (songGetStr (song, TAG_URI) == NULL) {
    *flags |= SONGDB_RET_BAD_URI;
    return 0;
  }

  stpecpy (olduri, olduri + sizeof (olduri), songGetStr (song, TAG_URI));
  *newuri = '\0';

  if (renameallow) {
    if (songdbNewName (songdb, song, newuri, sizeof (newuri))) {
      dorename = true;
    }
  }

  if (dorename && songGetNum (song, TAG_DB_LOC_LOCK) == true) {
    /* user requested location lock */
    *flags |= SONGDB_RET_LOC_LOCK;
    dorename = false;
  }

  if (dorename) {
    int     pfxlen;

    pfxlen = songGetNum (song, TAG_PREFIX_LEN);
    audiosrcFullPath (olduri, ffn, sizeof (ffn), olduri, pfxlen);
    /* the prefix length and old filename must be supplied */
    /* in order to generate the new filename properly */
    audiosrcFullPath (newuri, newffn, sizeof (newffn), olduri, pfxlen);

    if (*newffn && fileopFileExists (newffn)) {
      *flags |= SONGDB_RET_REN_FILE_EXISTS;
      dorename = false;
    }

    if (dorename) {
      pi = pathInfo (newffn);
      pathInfoGetDir (pi, tbuff, sizeof (tbuff));
      if (! fileopIsDirectory (tbuff)) {
        rc = diropMakeDir (tbuff);
        if (rc != 0) {
          logMsg (LOG_DBG, LOG_IMPORTANT, "unable to create dir %s", tbuff);
          dorename = false;
          *flags |= SONGDB_RET_RENAME_FAIL;
        }
      }
      pathInfoFree (pi);
    }

    if (dorename) {
      rc = filemanipMove (ffn, newffn);
      if (rc != 0) {
        logMsg (LOG_DBG, LOG_IMPORTANT, "unable to rename %s", ffn);
        logMsg (LOG_DBG, LOG_IMPORTANT, "  to %s", newffn);
        *flags |= SONGDB_RET_RENAME_FAIL;
        dorename = false;
      } else {
        *flags |= SONGDB_RET_RENAME_SUCCESS;
        logMsg (LOG_DBG, LOG_IMPORTANT, "rename %s", ffn);
        logMsg (LOG_DBG, LOG_IMPORTANT, "  to %s", newffn);
        renamesuccess = true;
      }
    }

    if (dorename && audiosrcOriginalExists (ffn)) {
      char    neworigffn [MAXPATHLEN];
      char    *p;
      char    *end;

      p = tbuff;
      end = tbuff + sizeof (tbuff);
      p = stpecpy (p, end, ffn);
      p = stpecpy (p, end, bdjvarsGetStr (BDJV_ORIGINAL_EXT));
      p = neworigffn;
      end = neworigffn + sizeof (neworigffn);
      p = stpecpy (p, end, newffn);
      p = stpecpy (p, end, bdjvarsGetStr (BDJV_ORIGINAL_EXT));
      rc = filemanipMove (tbuff, neworigffn);
      if (rc != 0) {
        logMsg (LOG_DBG, LOG_IMPORTANT, "unable to rename original %s %s", olduri, tbuff);
        *flags |= SONGDB_RET_ORIG_RENAME_FAIL;
      }
    }

    if (dorename && renamesuccess) {
      char    tdir [MAXPATHLEN];
      size_t  tdirlen;

      /* only reset the URI if the song was actually renamed */
      songSetStr (song, TAG_URI, newuri);
      if (pfxlen > 0) {
        /* for a secondary directory, the full name must be used for the URI */
        songSetStr (song, TAG_URI, newffn);
      }

      /* try to remove the old dir */
      tdirlen = audiosrcDir (ffn, tdir, sizeof (tdir), pfxlen);
      /* dirop-delete-dir works by removing the dir and all empty dirs */
      /* below it. it does not search the entire directory tree for empty */
      /* dirs, and will stop upon finding any non-empty dir. */
      /* work backwards from the old filename's path */
      pi = pathInfo (ffn);
      while (pi->dlen > tdirlen) {
        int     rc;

        snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->dlen, pi->dirname);
        rc = diropDeleteDir (tbuff, DIROP_ONLY_IF_EMPTY);
        if (rc == false) {
          /* if any files exist, don't try to go any further */
          break;
        }
        pathInfoFree (pi);
        stpecpy (dirbuff, dirbuff + sizeof (dirbuff), tbuff);
        pi = pathInfo (dirbuff);
      }
      pathInfoFree (pi);
    }
  }

  songSetNum (song, TAG_RRN, rrn);
  rrn = dbWriteSong (songdb->musicdb, song);
  if (rrn == MUSICDB_ENTRY_UNK) {
    *flags |= SONGDB_RET_WRITE_FAIL;
  } else {
    *flags |= SONGDB_RET_SUCCESS;
  }

  if (bdjoptGetNum (OPT_G_WRITETAGS) != WRITE_TAGS_NONE) {
    songdbWriteAudioTags (song, (*flags & SONGDB_FORCE_WRITE));
  }

  if (renamesuccess) {
    dbMarkEntryRenamed (songdb->musicdb, olduri, newuri,
        songGetNum (song, TAG_DBIDX));
  }

  if (songHasSonglistChange (song) || renamesuccess) {
    /* need to update all songlists if file/title/dance changed. */
    songdbUpdateAllSonglists (song, olduri);
  }
  songClearChanged (song);

  return rrn;
}

/* internal routines */

static bool
songdbNewName (songdb_t *songdb, song_t *song, char *newuri, size_t sz)
{
  char        ffn [MAXPATHLEN];
  const char  *songfname;
  const char  *bypass;
  char        *tnewfn;
  const char  *relfn;
  int         pfxlen;

  if (song == NULL) {
    return false;
  }
  if (songdb->org == NULL) {
    /* org_t have not been set up */
    return false;
  }

  *newuri = '\0';
  songfname = songGetStr (song, TAG_URI);
  audiosrcFullPath (songfname, ffn, sizeof (ffn),
      songfname, songGetNum (song, TAG_PREFIX_LEN));
  if (audiosrcGetType (ffn) != AUDIOSRC_TYPE_FILE) {
    return false;
  }
  if (songGetNum (song, TAG_DB_LOC_LOCK) == true) {
    return false;
  }

  bypass = "";
  if (songdb->orgold != NULL) {
    bypass = orgGetFromPath (songdb->orgold, songfname,
        (tagdefkey_t) ORG_TAG_BYPASS);
  }

  relfn = songfname;
  pfxlen = songGetNum (song, TAG_PREFIX_LEN);
  if (pfxlen > 0) {
    relfn = songfname + pfxlen;
  }
  tnewfn = orgMakeSongPath (songdb->org, song, bypass);
  if (strcmp (relfn, tnewfn) == 0) {
    /* no change */
    dataFree (tnewfn);
    return false;
  }

  stpecpy (newuri, newuri + sz, tnewfn);
  dataFree (tnewfn);

  return true;
}

static void
songdbWriteAudioTags (song_t *song, int32_t flags)
{
  const char  *fn;
  char        ffn [MAXPATHLEN];
  slist_t     *tagdata;
  slist_t     *newtaglist;
  int         rewrite;
  int32_t     atflags;

  fn = songGetStr (song, TAG_URI);
  if (audiosrcGetType (fn) != AUDIOSRC_TYPE_FILE) {
    /* for the time being, just ignore tagging other source types */
    return;
  }

  audiosrcFullPath (fn, ffn, sizeof (ffn),
      fn, songGetNum (song, TAG_PREFIX_LEN));
  tagdata = audiotagParseData (ffn, &rewrite);
  newtaglist = songTagList (song);
  atflags = AT_FLAGS_MOD_TIME_UPDATE;
  if ((flags & SONGDB_FORCE_WRITE) == SONGDB_FORCE_WRITE) {
    atflags |= AT_FLAGS_FORCE_WRITE;
  }
  audiotagWriteTags (ffn, tagdata, newtaglist, AF_REWRITE_NONE, atflags);
  slistFree (tagdata);
  slistFree (newtaglist);
}

static void
songdbUpdateAllSonglists (song_t *song, const char *olduri)
{
  slist_t     *filelist;
  slistidx_t  fiteridx;
  const char  *slfn;
  const char  *songuri;
  const char  *newuri = NULL;
  songlist_t  *songlist;
  ilistidx_t  sliter;

  if (olduri != NULL) {
    songuri = olduri;
    newuri = songGetStr (song, TAG_URI);
  } else {
    songuri = songGetStr (song, TAG_URI);
  }

  filelist = playlistGetPlaylistNames (PL_LIST_SONGLIST, NULL);
  slistStartIterator (filelist, &fiteridx);
  while ((slfn = slistIterateKey (filelist, &fiteridx)) != NULL) {
    ilistidx_t  key;
    bool        chg;
    const char  *tfn;

    chg = false;
    songlist = songlistLoad (slfn);
    songlistStartIterator (songlist, &sliter);
    while ((key = songlistIterate (songlist, &sliter)) >= 0) {
      tfn = songlistGetStr (songlist, key, SONGLIST_URI);
      if (tfn == NULL) {
        continue;
      }
      if (strcmp (tfn, songuri) == 0) {
        if (newuri != NULL) {
          songlistSetStr (songlist, key, SONGLIST_URI, newuri);
        }
        songlistSetStr (songlist, key, SONGLIST_TITLE,
            songGetStr (song, TAG_TITLE));
        songlistSetNum (songlist, key, SONGLIST_DANCE,
            songGetNum (song, TAG_DANCE));
        chg = true;
      }
    }
    if (chg) {
      songlistSave (songlist, SONGLIST_PRESERVE_TIMESTAMP, SONGLIST_USE_DIST_VERSION);
    }
    songlistFree (songlist);
  }
  slistFree (filelist);
}
