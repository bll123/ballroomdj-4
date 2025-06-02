/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "expimp.h"
#include "ilist.h"
#include "imppl.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "pathinfo.h"
#include "playlist.h"
#include "podcast.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"

typedef struct imppl {
  slist_t     *songidxlist;
  musicdb_t   *musicdb;
  char        *uri;
  char        *oplname;
  char        *plname;
  asiter_t    *asiter;
  int         imptype;
  int         askey;
  int         retain;
  int         count;
  int         tot;
  bool        newsongs;
  bool        batched;
} imppl_t;

static bool impplCreateSong (imppl_t *imppl, const char *songnm, slist_t *tagdata);

imppl_t *
impplInit (slist_t *songidxlist, musicdb_t *musicdb, int imptype,
    const char *uri, const char *oplname, const char *plname, int askey)
{
  imppl_t   *imppl;

  if (uri == NULL || ! *uri || musicdb == NULL) {
    return NULL;
  }
  if (oplname == NULL || plname == NULL) {
    return NULL;
  }

  imppl = mdmalloc (sizeof (imppl_t));
  imppl->songidxlist = songidxlist;
  imppl->musicdb = musicdb;
  imppl->uri = mdstrdup (uri);
  imppl->oplname = mdstrdup (oplname);
  imppl->plname = mdstrdup (plname);
  imppl->asiter = NULL;
  imppl->imptype = imptype;
  imppl->askey = askey;
  imppl->retain = 0;
  imppl->newsongs = false;
  imppl->batched = false;
  imppl->count = 0;
  imppl->tot = 0;

  if (imptype == AUDIOSRC_TYPE_BDJ4 ||
      imptype == AUDIOSRC_TYPE_PODCAST) {
    if (imppl->imptype == AUDIOSRC_TYPE_PODCAST) {
      playlist_t    *pl;

      pl = playlistLoad (imppl->plname, NULL, NULL);
      if (pl != NULL) {
        imppl->retain = playlistGetPodcastNum (pl, PODCAST_RETAIN);
      }
      playlistFree (pl);
    }

    imppl->asiter = audiosrcStartIterator (imppl->imptype,
        AS_ITER_PL, imppl->uri, imppl->oplname, imppl->askey);
    imppl->tot = audiosrcIterCount (imppl->asiter);
    dbStartBatch (imppl->musicdb);
    imppl->batched = true;
  }

  return imppl;
}

void
impplFree (imppl_t *imppl)
{
  if (imppl == NULL) {
    return;
  }

  if (imppl->batched) {
    dbEndBatch (imppl->musicdb);
  }
  audiosrcCleanIterator (imppl->asiter);
  dataFree (imppl->uri);
  dataFree (imppl->oplname);
  dataFree (imppl->plname);
  mdfree (imppl);
}

bool
impplProcess (imppl_t *imppl)
{
  if (imppl == NULL) {
    return true;
  }

  if (imppl->imptype == AUDIOSRC_TYPE_FILE) {
    nlist_t     *list = NULL;
    nlistidx_t  iteridx;
    pathinfo_t  *pi;
    dbidx_t     dbidx;

    pi = pathInfo (imppl->uri);

    if (pathInfoExtCheck (pi, ".m3u") || pathInfoExtCheck (pi, ".m3u8")) {
      list = m3uImport (imppl->musicdb, imppl->uri);
    }
    if (pathInfoExtCheck (pi, ".xspf")) {
      list = xspfImport (imppl->musicdb, imppl->uri);
    }
    if (pathInfoExtCheck (pi, ".jspf")) {
      list = jspfImport (imppl->musicdb, imppl->uri);
    }

    pathInfoFree (pi);

    if (list != NULL && nlistGetCount (list) > 0) {
      nlistStartIterator (list, &iteridx);
      while ((dbidx = nlistIterateKey (list, &iteridx)) >= 0) {
        song_t    *song;

        song = dbGetByIdx (imppl->musicdb, dbidx);
        if (song == NULL) {
          continue;
        }
        slistSetNum (imppl->songidxlist, songGetStr (song, TAG_URI), dbidx);
      }
    }
    nlistFree (list);
    /* importing from a file is fast, just do it all */
    return true;
  } /* audiosrc-type: file */

  if (imppl->imptype == AUDIOSRC_TYPE_BDJ4 ||
      imppl->imptype == AUDIOSRC_TYPE_PODCAST) {
    const char  *songnm;
    slist_t     *tagdata;
    asiter_t    *tagiter;
    const char  *tag;

    songnm = audiosrcIterate (imppl->asiter);
    if (songnm == NULL) {
      return true;
    }

    imppl->count += 1;

    tagdata = slistAlloc ("asimppl-bdj4", LIST_UNORDERED, NULL);

    tagiter = audiosrcStartIterator (imppl->imptype,
        AS_ITER_TAGS, imppl->uri, songnm, imppl->askey);
    while ((tag = audiosrcIterate (tagiter)) != NULL) {
      const char  *tval;

      tval = audiosrcIterateValue (tagiter, tag);
      slistSetStr (tagdata, tag, tval);
    }
    audiosrcCleanIterator (tagiter);

    slistSetStr (tagdata, tagdefs [TAG_URI].tag, songnm);
    slistSort (tagdata);

    imppl->newsongs |= impplCreateSong (imppl, songnm, tagdata);
    slistFree (tagdata);
  }

  /* not yet done */
  return false;
}

bool
impplHaveNewSongs (imppl_t *imppl)
{
  if (imppl == NULL) {
    return false;
  }

  return imppl->newsongs;
}

void
impplGetCount (imppl_t *imppl, int *count, int *tot)
{
  if (imppl == NULL) {
    return;
  }

  *count = imppl->count;
  *tot = imppl->tot;
}

void
impplFinalize (imppl_t *imppl)
{
  if (imppl == NULL) {
    return;
  }

  if (imppl->imptype == AUDIOSRC_TYPE_PODCAST) {
    podcast_t   *podcast;
    bool        podcastexists = true;

    podcast = podcastLoad (imppl->plname);
    if (podcast == NULL) {
      podcast = podcastCreate (imppl->plname);
      podcastexists = false;
    }

    podcastSetStr (podcast, PODCAST_URI, imppl->uri);
    podcastSetStr (podcast, PODCAST_TITLE, imppl->plname);
// ### need to get last-build-date from audio-src ?
    if (podcastexists == false) {
      podcastSetNum (podcast, PODCAST_RETAIN, 0);
    }
    podcastSave (podcast);
    podcastFree (podcast);
  }
}


static bool
impplCreateSong (imppl_t *imppl, const char *songnm, slist_t *tagdata)
{
  song_t    *song = NULL;
  bool      rc = false;
  dbidx_t   dbidx;


  song = dbGetByName (imppl->musicdb, songnm);
  if (song == NULL) {
    /* song needs to be added */
    song = songAlloc ();
    songFromTagList (song, tagdata);
    songSetNum (song, TAG_DB_FLAGS, MUSICDB_STD);
    songSetNum (song, TAG_RRN, MUSICDB_ENTRY_NEW);
    songSetNum (song, TAG_PREFIX_LEN, 0);
    rc = true;
  }

  if (imppl->retain > 0) {
    time_t      currtm;
    time_t      podtm;
    time_t      days = 0;

    currtm = time (NULL);
    podtm = songGetNum (song, TAG_DBADDDATE);
    days = currtm - podtm;
    days /= 24 * 3600;
    if (days > imppl->retain) {
      if (rc == false) {
        /* only if the song is not new */
        dbidx = songGetNum (song, TAG_DBIDX);
        dbRemoveSong (imppl->musicdb, dbidx);
      }
      return false;
    }
  }

  if (rc) {
    dbWriteSong (imppl->musicdb, song);
  }

  slistSetNum (imppl->songidxlist, songnm, -1);
  return rc;
}
