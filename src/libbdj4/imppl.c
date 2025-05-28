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

static bool impplCreateSong (musicdb_t *musicdb, const char *songnm, int imptype, slist_t *songidxlist, slist_t *tagdata, int retain);

/* only handles BDJ4 and Podcast import types types */
bool
impplPlaylistImport (slist_t *songidxlist, musicdb_t *musicdb, int imptype,
    const char *uri, const char *oplname, const char *plname, int askey)
{
  bool        newsongs = false;

  if (! *uri) {
    return false;
  }

  if (imptype == AUDIOSRC_TYPE_FILE) {
    nlist_t     *list = NULL;
    nlistidx_t  iteridx;
    pathinfo_t  *pi;
    dbidx_t     dbidx;

    pi = pathInfo (uri);

    if (pathInfoExtCheck (pi, ".m3u") || pathInfoExtCheck (pi, ".m3u8")) {
      list = m3uImport (musicdb, uri);
    }
    if (pathInfoExtCheck (pi, ".xspf")) {
      list = xspfImport (musicdb, uri);
    }
    if (pathInfoExtCheck (pi, ".jspf")) {
      list = jspfImport (musicdb, uri);
    }

    pathInfoFree (pi);

    if (list != NULL && nlistGetCount (list) > 0) {
      nlistStartIterator (list, &iteridx);
      while ((dbidx = nlistIterateKey (list, &iteridx)) >= 0) {
        song_t    *song;

        song = dbGetByIdx (musicdb, dbidx);
        if (song == NULL) {
          continue;
        }
        slistSetNum (songidxlist, songGetStr (song, TAG_URI), dbidx);
      }
    }
    nlistFree (list);
    return false;
  } /* audiosrc-type: file */

  if (imptype == AUDIOSRC_TYPE_BDJ4 || imptype == AUDIOSRC_TYPE_PODCAST) {
    asiter_t    *asiter;
    const char  *songnm;
    int         retain = 0;

    if (imptype == AUDIOSRC_TYPE_PODCAST) {
      playlist_t    *pl;

      pl = playlistLoad (plname, NULL, NULL);
      if (pl != NULL) {
        retain = playlistGetPodcastNum (pl, PODCAST_RETAIN);
      }
      playlistFree (pl);
    }

    asiter = audiosrcStartIterator (imptype, AS_ITER_PL, uri, oplname, askey);
    dbStartBatch (musicdb);
    while ((songnm = audiosrcIterate (asiter)) != NULL) {
      slist_t     *tagdata;
      asiter_t    *tagiter;
      const char  *tag;

      tagdata = slistAlloc ("asimppl-bdj4", LIST_UNORDERED, NULL);

      tagiter = audiosrcStartIterator (imptype, AS_ITER_TAGS, uri, songnm, askey);
      while ((tag = audiosrcIterate (tagiter)) != NULL) {
        const char  *tval;

        tval = audiosrcIterateValue (tagiter, tag);
        slistSetStr (tagdata, tag, tval);
      }
      audiosrcCleanIterator (tagiter);

      slistSetStr (tagdata, tagdefs [TAG_URI].tag, songnm);
      slistSort (tagdata);

      newsongs |= impplCreateSong (
          musicdb, songnm, imptype, songidxlist, tagdata, retain);
      slistFree (tagdata);
    }
    dbEndBatch (musicdb);
    audiosrcCleanIterator (asiter);
  }

  if (imptype == AUDIOSRC_TYPE_PODCAST) {
    podcast_t   *podcast;
    bool        podcastexists = true;

    podcast = podcastLoad (plname);
    if (podcast == NULL) {
      podcast = podcastCreate (plname);
      podcastexists = false;
    }

    podcastSetStr (podcast, PODCAST_URI, uri);
    podcastSetStr (podcast, PODCAST_TITLE, plname);
// ### need to get last-build-date from audio-src ?
    if (podcastexists == false) {
      podcastSetNum (podcast, PODCAST_RETAIN, 0);
    }
    podcastSave (podcast);
    podcastFree (podcast);
  }

  return newsongs;
}

static bool
impplCreateSong (musicdb_t *musicdb, const char *songnm,
    int imptype, slist_t *songidxlist, slist_t *tagdata, int retain)
{
  song_t    *song = NULL;
  bool      rc = false;
  dbidx_t   dbidx;


  song = dbGetByName (musicdb, songnm);
  if (song == NULL) {
    /* song needs to be added */
    song = songAlloc ();
    songFromTagList (song, tagdata);
    songSetNum (song, TAG_DB_FLAGS, MUSICDB_STD);
    songSetNum (song, TAG_RRN, MUSICDB_ENTRY_NEW);
    songSetNum (song, TAG_PREFIX_LEN, 0);
    rc = true;
  }

  if (retain > 0) {
    time_t      currtm;
    time_t      podtm;
    time_t      days = 0;

    currtm = time (NULL);
    podtm = songGetNum (song, TAG_DBADDDATE);
    days = currtm - podtm;
    days /= 24 * 3600;
    if (days > retain) {
      if (rc == false) {
        /* only if the song is not new */
        dbidx = songGetNum (song, TAG_DBIDX);
        dbRemoveSong (musicdb, dbidx);
      }
      return false;
    }
  }

  if (rc) {
    dbWriteSong (musicdb, song);
  }

  slistSetNum (songidxlist, songnm, -1);
  return rc;
}
