/*
 * Copyright 2025-2026 Brad Lanam Pleasant Hill CA
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
#include "nodiscard.h"
#include "pathinfo.h"
#include "podcast.h"
#include "podcastutil.h"
#include "slist.h"
#include "song.h"
#include "songlist.h"
#include "tagdef.h"

typedef enum {
    IMPPL_INIT_PODCAST,
    IMPPL_PROCESS,
    IMPPL_INIT_EXISTING,
    IMPPL_PROCESS_EXISTING,
    IMPPL_PROCESS_FINALIZE,
} impplstate_t;

typedef struct imppl {
  slist_t     *songidxlist;
  slist_t     *tsongidxlist;
  slist_t     *sortlist;
  musicdb_t   *musicdb;
  char        *uri;
  char        *oplname;
  char        *plname;
  asiter_t    *asiter;
  slistidx_t  tsiter;
  int         imptype;
  int         askey;
  int         retain;
  int         count;
  int         tot;
  /* processing state */
  int         state;
  bool        dbchanged;
  bool        dbbatched;
} imppl_t;

static bool impplCreateSong (imppl_t *imppl, const char *songuri, slist_t *tagdata);

NODISCARD
imppl_t *
impplInit (slist_t *songidxlist, int imptype,
    const char *uri, const char *oplname, const char *plname, int askey)
{
  imppl_t   *imppl;

  if (uri == NULL || ! *uri) {
    return NULL;
  }
  if (oplname == NULL || plname == NULL) {
    return NULL;
  }

  imppl = mdmalloc (sizeof (imppl_t));
  imppl->songidxlist = songidxlist;
  imppl->tsongidxlist = slistAlloc ("imppl-tsongidxlist", LIST_UNORDERED, NULL);
  imppl->sortlist = NULL;
  imppl->musicdb = NULL;
  imppl->uri = mdstrdup (uri);
  imppl->oplname = mdstrdup (oplname);
  imppl->plname = mdstrdup (plname);
  imppl->asiter = NULL;
  imppl->imptype = imptype;
  imppl->askey = askey;
  imppl->retain = 0;
  imppl->dbchanged = false;
  imppl->dbbatched = false;
  imppl->count = 0;
  imppl->tot = 0;
  imppl->state = IMPPL_PROCESS;

  if (imptype == AUDIOSRC_TYPE_BDJ4 ||
      imptype == AUDIOSRC_TYPE_PODCAST) {
    if (imptype == AUDIOSRC_TYPE_PODCAST) {
      imppl->state = IMPPL_INIT_PODCAST;
    }
    imppl->asiter = audiosrcStartIterator (imppl->imptype,
        AS_ITER_PL, imppl->uri, imppl->oplname, imppl->askey);
    imppl->tot = audiosrcIterCount (imppl->asiter);
    dbStartBatch (imppl->musicdb);
    imppl->dbbatched = true;
  }

  return imppl;
}

void
impplFree (imppl_t *imppl)
{
  if (imppl == NULL) {
    return;
  }

  if (imppl->dbbatched) {
    dbEndBatch (imppl->musicdb);
  }
  audiosrcCleanIterator (imppl->asiter);
  dataFree (imppl->uri);
  dataFree (imppl->oplname);
  dataFree (imppl->plname);
  slistFree (imppl->tsongidxlist);
  slistFree (imppl->sortlist);
  mdfree (imppl);
}

bool
impplProcess (imppl_t *imppl)
{
  if (imppl == NULL || imppl->musicdb == NULL) {
    return true;
  }

  if (imppl->state == IMPPL_INIT_PODCAST) {
    podcast_t     *podcast;
    songlist_t    *sl;
    ilistidx_t    sliteridx;
    ilistidx_t    slkey;

    podcast = podcastLoad (imppl->plname);
    if (podcast != NULL) {
      imppl->retain = podcastGetNum (podcast, PODCAST_RETAIN);
    }
    podcastFree (podcast);

    sl = songlistLoad (imppl->plname);
    slistSetSize (imppl->tsongidxlist, songlistGetCount (sl));
    songlistStartIterator (sl, &sliteridx);
    while ((slkey = songlistIterate (sl, &sliteridx)) >= 0) {
      const char  *songuri;
      song_t      *song;

      songuri = songlistGetStr (sl, slkey, SONGLIST_URI);
      song = dbGetByName (imppl->musicdb, songuri);
      if (song != NULL) {
        /* the dbidx may get changed due to database additions */
        /* or removals, don't bother saving it */
        slistSetNum (imppl->tsongidxlist, songuri, -1);
      }
    }
    songlistFree (sl);
    imppl->tot += slistGetCount (imppl->tsongidxlist);
    slistSetSize (imppl->songidxlist, imppl->tot);
    imppl->state = IMPPL_PROCESS;
    return false;
  }

  if (imppl->state == IMPPL_INIT_EXISTING) {
    const char    *songuri;
    slistidx_t    iteridx;

    /* need a sorted list to do lookups */
    imppl->sortlist = slistAlloc ("imppl-tlist", LIST_UNORDERED, NULL);
    slistSetSize (imppl->sortlist, slistGetCount (imppl->songidxlist));
    slistStartIterator (imppl->songidxlist, &iteridx);
    while ((songuri = slistIterateKey (imppl->songidxlist, &iteridx)) != NULL) {
      slistSetNum (imppl->sortlist, songuri, -1);
    }
    slistSort (imppl->sortlist);

    slistStartIterator (imppl->tsongidxlist, &imppl->tsiter);

    imppl->state = IMPPL_PROCESS_EXISTING;
    return false;
  }

  if (imppl->state == IMPPL_PROCESS_EXISTING) {
    dbidx_t     dbidx;
    slistidx_t  tidx;
    pcretain_t  pcrc;
    song_t      *song;
    const char  *songuri;

    /* not perfect */
    /* an older item in the podcast that is still loaded in the new list */
    /* does not get sorted by date. */

    songuri = slistIterateKey (imppl->tsongidxlist, &imppl->tsiter);
    if (songuri == NULL) {
      imppl->state = IMPPL_PROCESS_FINALIZE;
      /* finished */
      return true;
    }

    imppl->count += 1;

    tidx = slistGetIdx (imppl->sortlist, songuri);
    if (tidx >= 0) {
      /* song already exists in the newly built songidxlist */
      /* any song in the new songidxlist has already been checked for */
      /* the retain date */
      return false;
    }

    song = dbGetByName (imppl->musicdb, songuri);
    dbidx = songGetNum (song, TAG_DBIDX);
    pcrc = podcastutilCheckRetain (song, imppl->retain);
    if (pcrc == PODCAST_DELETE) {
      /* remove any songs past the retain date */
      dbRemoveSong (imppl->musicdb, dbidx);
      imppl->dbchanged = true;
      return false;
    }

    slistSetNum (imppl->songidxlist, songuri, -1);
    return false;
  }

  if (imppl->state != IMPPL_PROCESS) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: imppl: invalid state");
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
    const char  *songuri;
    slist_t     *tagdata;
    asiter_t    *tagiter;
    const char  *tag;

    songuri = audiosrcIterate (imppl->asiter);
    if (songuri == NULL) {
      imppl->state = IMPPL_PROCESS_FINALIZE;
      if (imppl->imptype == AUDIOSRC_TYPE_PODCAST) {
        imppl->state = IMPPL_INIT_EXISTING;
        return false;
      }
      return true;
    }

    imppl->count += 1;

    tagdata = slistAlloc ("asimppl-bdj4", LIST_UNORDERED, NULL);

    tagiter = audiosrcStartIterator (imppl->imptype,
        AS_ITER_TAGS, imppl->uri, songuri, imppl->askey);
    while ((tag = audiosrcIterate (tagiter)) != NULL) {
      const char  *tval;

      tval = audiosrcIterateValue (tagiter, tag);
      slistSetStr (tagdata, tag, tval);
    }
    audiosrcCleanIterator (tagiter);

    slistSetStr (tagdata, tagdefs [TAG_URI].tag, songuri);
    slistSort (tagdata);

    imppl->dbchanged |= impplCreateSong (imppl, songuri, tagdata);
    slistFree (tagdata);
  }

  /* not yet done */
  return false;
}

bool
impplIsDBChanged (imppl_t *imppl)
{
  if (imppl == NULL || imppl->musicdb == NULL) {
    return false;
  }

  return imppl->dbchanged;
}

void
impplGetCount (imppl_t *imppl, int *count, int *tot)
{
  if (imppl == NULL || imppl->musicdb == NULL) {
    return;
  }

  *count = imppl->count;
  *tot = imppl->tot;
}

int
impplGetType (imppl_t *imppl)
{
  if (imppl == NULL || imppl->musicdb == NULL) {
    return AUDIOSRC_TYPE_NONE;
  }

  return imppl->imptype;
}

void
impplSetDB (imppl_t *imppl, musicdb_t *musicdb)
{
  if (imppl == NULL) {
    return;
  }

  imppl->musicdb = musicdb;
}

void
impplFinalize (imppl_t *imppl)
{
  if (imppl == NULL || imppl->musicdb == NULL) {
    return;
  }

  if (imppl->imptype == AUDIOSRC_TYPE_PODCAST) {
    podcast_t   *podcast;
    bool        podcastexists = true;
    asiter_t    *pldataiter;
    const char  *podtag;

    podcast = podcastLoad (imppl->plname);
    if (podcast == NULL) {
      podcast = podcastCreate (imppl->plname);
      podcastexists = false;
    }

    podcastSetStr (podcast, PODCAST_URI, imppl->uri);
    podcastSetStr (podcast, PODCAST_TITLE, imppl->plname);
    pldataiter = audiosrcStartIterator (imppl->imptype,
        AS_ITER_PL_DATA, imppl->uri, imppl->oplname, imppl->askey);
    while ((podtag = audiosrcIterate (pldataiter)) != NULL) {
      const char  *tval;

      tval = audiosrcIterateValue (pldataiter, podtag);
      /* someday this should be redone to table driven */
      if (strcmp (podtag, "IMAGE_URI") == 0) {
        podcastSetStr (podcast, PODCAST_IMAGE_URI, tval);
      }
    }
    if (podcastexists == false) {
      podcastSetNum (podcast, PODCAST_RETAIN, 0);
    }
    podcastSave (podcast);
    podcastFree (podcast);
  }
}


static bool
impplCreateSong (imppl_t *imppl, const char *songuri, slist_t *tagdata)
{
  song_t      *song = NULL;
  bool        exists = false;
  dbidx_t     dbidx;
  pcretain_t  pcrc;
  bool        dbchg = false;


  song = dbGetByName (imppl->musicdb, songuri);
  exists = true;
  if (song == NULL) {
    /* song needs to be added */
    song = songAlloc ();
    songFromTagList (song, tagdata);
    songSetNum (song, TAG_DB_FLAGS, MUSICDB_STD);
    songSetNum (song, TAG_RRN, MUSICDB_ENTRY_NEW);
    songSetNum (song, TAG_PREFIX_LEN, 0);
    exists = false;
  }

  pcrc = podcastutilCheckRetain (song, imppl->retain);
  if (pcrc == PODCAST_DELETE) {
    if (! exists) {
      /* song is new, but is not retained */
      /* database change flag is false */
      songFree (song);
    }
    if (exists) {
      /* only if the song is not new */
      dbidx = songGetNum (song, TAG_DBIDX);
      dbRemoveSong (imppl->musicdb, dbidx);
      /* database is changed */
      dbchg = true;
    }
    return dbchg;
  }

  if (! exists) {
    dbWriteSong (imppl->musicdb, song);
    /* new song, database is changed */
    dbchg = true;
  }

  slistSetNum (imppl->songidxlist, songuri, -1);
  return dbchg;
}
