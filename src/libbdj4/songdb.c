/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

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
#include "pathutil.h"
#include "playlist.h"
#include "slist.h"
#include "song.h"
#include "songdb.h"
#include "songlist.h"
#include "tagdef.h"

static void songdbWriteAudioTags (song_t *song);
static void songdbUpdateAllSonglists (song_t *song, const char *olduri);

typedef struct songdb {
  musicdb_t *musicdb;
  org_t     *org;
  org_t     *orgold;
} songdb_t;

songdb_t *
songdbAlloc (musicdb_t *musicdb)
{
  songdb_t *songdb;
  const char  *torgpath;

  songdb = mdmalloc (sizeof (songdb_t));
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
  if (songdb == NULL) {
    return;
  }

  orgFree (songdb->org);
  songdb->org = NULL;
  orgFree (songdb->orgold);
  mdfree (songdb);
}

void
songdbSetMusicDB (songdb_t *songdb, musicdb_t *musicdb)
{
  if (songdb == NULL) {
    return;
  }

  songdb->musicdb = musicdb;
}

void
songdbWriteDB (songdb_t *songdb, dbidx_t dbidx)
{
  song_t    *song;
  int       songdbflags;

  if (songdb == NULL || songdb->musicdb == NULL) {
    return;
  }
  song = dbGetByIdx (songdb->musicdb, dbidx);
  songdbflags = SONGDB_NONE;
  songdbWriteDBSong (songdb, song, &songdbflags, songGetNum (song, TAG_RRN));
}

/* flags is both input and output */
size_t
songdbWriteDBSong (songdb_t *songdb, song_t *song, int *flags, dbidx_t rrn)
{
  char        newfn [MAXPATHLEN];
  char        newffn [MAXPATHLEN];
  char        oldfn [MAXPATHLEN];
  char        ffn [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  bool        dorename = false;
  bool        rename = false;
  pathinfo_t  *pi;
  int         rc;
  size_t      len;

  if (songdb == NULL || songdb->musicdb == NULL) {
    *flags |= SONGDB_RET_NULL;
    return 0;
  }
  if (song == NULL) {
    *flags |= SONGDB_RET_NULL;
    return 0;
  }
  if (! songIsChanged (song)) {
    *flags |= SONGDB_RET_NO_CHANGE;
    return 0;
  }

  rename = (bdjoptGetNum (OPT_G_AUTOORGANIZE) == true) ||
      ((*flags & SONGDB_FORCE_RENAME) == SONGDB_FORCE_RENAME);

  strlcpy (oldfn, songGetStr (song, TAG_URI), sizeof (oldfn));
  *newfn = '\0';

  if (rename) {
    if (songdbNewName (songdb, song, newfn, sizeof (newfn))) {
      strlcpy (oldfn, songGetStr (song, TAG_URI), sizeof (oldfn));
      songSetStr (song, TAG_URI, newfn);
      dorename = true;
    }
  }

  if (songGetNum (song, TAG_DB_LOC_LOCK) == true) {
    /* user requested location lock */
    *flags |= SONGDB_RET_LOC_LOCK;
    dorename = false;
  }

  if (dorename) {
    audiosrcFullPath (oldfn, ffn, sizeof (ffn));
    audiosrcFullPath (newfn, newffn, sizeof (newffn));

    if (*newffn && fileopFileExists (newffn)) {
      *flags |= SONGDB_RET_REN_FILE_EXISTS;
      dorename = false;
    }

    if (dorename) {
      pi = pathInfo (newffn);
      snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->dlen, pi->dirname);
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
        logMsg (LOG_DBG, LOG_IMPORTANT, "unable to rename %s %s", oldfn, newffn);
        *flags |= SONGDB_RET_RENAME_FAIL;
      } else {
        *flags |= SONGDB_RET_RENAME_SUCCESS;

        /* try to remove the old dir */
        pi = pathInfo (ffn);
        snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->dlen, pi->dirname);
        pathInfoFree (pi);
      }
    }

    if (dorename && audiosrcOriginalExists (ffn)) {
      strlcpy (tbuff, ffn, sizeof (tbuff));
      strlcat (tbuff, bdjvarsGetStr (BDJV_ORIGINAL_EXT), sizeof (tbuff));
      strlcat (newffn, bdjvarsGetStr (BDJV_ORIGINAL_EXT), sizeof (newffn));
      rc = filemanipMove (tbuff, newffn);
      if (rc != 0) {
        logMsg (LOG_DBG, LOG_IMPORTANT, "unable to rename original %s %s", oldfn, tbuff);
        *flags |= SONGDB_RET_ORIG_RENAME_FAIL;
      }
    }
  }

  *flags |= SONGDB_RET_SUCCESS;

  songSetNum (song, TAG_RRN, rrn);
  len = dbWriteSong (songdb->musicdb, song);

  if (bdjoptGetNum (OPT_G_WRITETAGS) != WRITE_TAGS_NONE) {
    songdbWriteAudioTags (song);
  }
  if (songHasSonglistChange (song) || dorename) {
    /* need to update all songlists if file/title/dance changed. */
    songdbUpdateAllSonglists (song, oldfn);
  }
  songClearChanged (song);

  return len;
}

bool
songdbNewName (songdb_t *songdb, song_t *song, char *newuri, size_t sz)
{
  char        ffn [MAXPATHLEN];
  const char  *songfname;
  const char  *bypass;
  char        *tnewfn;

  if (song == NULL) {
    return false;
  }
  if (songdb->org == NULL) {
    /* org_t have not been set up */
    return false;
  }

  *newuri = '\0';
  songfname = songGetStr (song, TAG_URI);
  audiosrcFullPath (songfname, ffn, sizeof (ffn));
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

  tnewfn = orgMakeSongPath (songdb->org, song, bypass);
  if (strcmp (tnewfn, songfname) == 0) {
    /* no change */
    dataFree (tnewfn);
    return false;
  }

  strlcpy (newuri, tnewfn, sz);
  dataFree (tnewfn);

  return true;
}

/* internal routines */

static void
songdbWriteAudioTags (song_t *song)
{
  const char  *fn;
  char        ffn [MAXPATHLEN];
  slist_t     *tagdata;
  slist_t     *newtaglist;
  int         rewrite;

  fn = songGetStr (song, TAG_URI);
  if (audiosrcGetType (fn) != AUDIOSRC_TYPE_FILE) {
    /* for the time being, just ignore tagging other source types */
    return;
  }

  audiosrcFullPath (fn, ffn, sizeof (ffn));
  tagdata = audiotagParseData (ffn, &rewrite);
  newtaglist = songTagList (song);
  audiotagWriteTags (ffn, tagdata, newtaglist, AF_REWRITE_NONE, AT_UPDATE_MOD_TIME);
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
  filelist = playlistGetPlaylistList (PL_LIST_SONGLIST, NULL);
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
}
