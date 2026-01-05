/*
 * Copyright 2025-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "asconf.h"
#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjvarsdf.h"
#include "imppl.h"
#include "log.h"
#include "mdebug.h"
#include "playlist.h"
#include "podcast.h"
#include "podcastutil.h"
#include "slist.h"
#include "songlist.h"
#include "songlistutil.h"
#include "tagdef.h"
#include "tmutil.h"

typedef struct {
  char      *plname;
  slist_t   *songidxlist;
} pcupditem_t;

typedef struct {
  pcupditem_t *itemlist;
  musicdb_t   *musicdb;
  int         idx;
  int         count;
  int         askey;
  bool        dbchanged;
} pcupd_t;

static int podcastupdGetASKey (void);
static void podcastupdProcess (pcupd_t *pcupd);
static bool podcastupdProcessPodcast (pcupd_t *pcupd, const char *plname);
static void podcastupdCreateSonglists (pcupd_t *pcupd);

int
main (int argc, char *argv[])
{
  uint32_t    flags;
  slist_t     *filelist;
  pcupd_t     pcupd;

#if BDJ4_MEM_DEBUG
  mdebugInit ("podu");
#endif

  pcupd.itemlist = NULL;
  pcupd.musicdb = NULL;
  pcupd.count = 0;
  pcupd.idx = 0;
  pcupd.askey = -1;
  pcupd.dbchanged = false;

  /* do the startup w/o the DB first */
  flags = BDJ4_INIT_NO_DB_LOAD;
  bdj4startup (argc, argv, NULL, "podu", ROUTE_PODCASTUPD, &flags);
  logProcBegin ();

  filelist = playlistGetPlaylistNames (PL_LIST_PODCAST, NULL);
  pcupd.count = slistGetCount (filelist);
  pcupd.askey = podcastupdGetASKey ();
  bdj4shutdown (ROUTE_PODCASTUPD, NULL);
  slistFree (filelist);

  if (pcupd.count > 0 && pcupd.askey >= 0) {
    pcupd.itemlist = mdmalloc (sizeof (pcupditem_t) * pcupd.count);
    for (int idx = 0; idx < pcupd.count; ++idx) {
      pcupd.itemlist [idx].plname = NULL;
      pcupd.itemlist [idx].songidxlist = NULL;
    }

    logEnd ();

    /* the database now needs to be loaded */
    flags = BDJ4_INIT_ALL;
    bdj4startup (argc, argv, &pcupd.musicdb, "podu", ROUTE_PODCASTUPD, &flags);

    podcastupdProcess (&pcupd);

    bdj4shutdown (ROUTE_PODCASTUPD, pcupd.musicdb);

    for (int idx = 0; idx < pcupd.count; ++idx) {
      slistFree (pcupd.itemlist [idx].songidxlist);
      mdfree (pcupd.itemlist [idx].plname);
    }
    mdfree (pcupd.itemlist);
  }

  logProcEnd ("");
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

static int
podcastupdGetASKey (void)
{
  asconf_t    *asconf;
  ilistidx_t  askey = -1;
  ilistidx_t  taskey = -1;
  ilistidx_t  iteridx;

  asconf = bdjvarsdfGet (BDJVDF_AUDIOSRC_CONF);
  asconfStartIterator (asconf, &iteridx);
  while ((taskey = asconfIterate (asconf, &iteridx)) >= 0) {
    if (asconfGetNum (asconf, taskey, ASCONF_MODE) == ASCONF_MODE_CLIENT) {
      int   type;

      type = asconfGetNum (asconf, taskey, ASCONF_TYPE);
      if (type == AUDIOSRC_TYPE_PODCAST) {
        askey = taskey;
        break;
      }
    }
  }

  return askey;
}

static void
podcastupdProcess (pcupd_t *pcupd)
{
  slist_t     *filelist;
  slistidx_t  iteridx;
  const char  *nm;

  filelist = playlistGetPlaylistNames (PL_LIST_PODCAST, NULL);
  slistStartIterator (filelist, &iteridx);
  while ((nm = slistIterateKey (filelist, &iteridx)) != NULL) {
    pcupd->dbchanged |= podcastupdProcessPodcast (pcupd, nm);
    pcupd->idx += 1;
  }
  slistFree (filelist);

  podcastupdCreateSonglists (pcupd);
}

static bool
podcastupdProcessPodcast (pcupd_t *pcupd, const char *plname)
{
  podcast_t   *podcast;
  int         retain;
  imppl_t     *imppl;
  bool        dbchanged;

  logMsg (LOG_DBG, LOG_INFO, "process %s", plname);

  podcast = podcastLoad (plname);
  if (podcast == NULL) {
    return false;
  }

  retain = podcastGetNum (podcast, PODCAST_RETAIN);

  pcupd->itemlist [pcupd->idx].plname = mdstrdup (plname);
  pcupd->itemlist [pcupd->idx].songidxlist =
      slistAlloc ("podu-imppl-song-idx", LIST_UNORDERED, NULL);
  imppl = impplInit (pcupd->itemlist [pcupd->idx].songidxlist,
      AUDIOSRC_TYPE_PODCAST,
      podcastGetStr (podcast, PODCAST_URI), plname, plname, pcupd->askey);
  impplSetDB (imppl, pcupd->musicdb);
  while (impplProcess (imppl) == false) {
    ;
  }

  dbchanged = impplIsDBChanged (imppl);
  if (dbchanged) {
    pcupd->musicdb = bdj4ReloadDatabase (pcupd->musicdb);
    impplSetDB (imppl, pcupd->musicdb);
  }

  impplFinalize (imppl);

  podcastFree (podcast);
  impplFree (imppl);

  return dbchanged;
}

static void
podcastupdCreateSonglists (pcupd_t *pcupd)
{
  for (int idx = 0; idx < pcupd->count; ++idx) {
    nlist_t     *dbidxlist;
    const char  *songuri;
    slistidx_t  iteridx;

    dbidxlist = nlistAlloc ("tmp-pcupd-dbidx", LIST_UNORDERED, NULL);
    nlistSetSize (dbidxlist, slistGetCount (pcupd->itemlist [idx].songidxlist));

    slistStartIterator (pcupd->itemlist [idx].songidxlist, &iteridx);
    while ((songuri =
        slistIterateKey (pcupd->itemlist [idx].songidxlist, &iteridx)) != NULL) {
      dbidx_t     dbidx;

      dbidx = slistGetNum (pcupd->itemlist [idx].songidxlist, songuri);
      if (dbidx < 0) {
        song_t    *song;

        song = dbGetByName (pcupd->musicdb, songuri);
        if (song == NULL) {
          continue;
        }

        dbidx = songGetNum (song, TAG_DBIDX);
      }

      nlistSetNum (dbidxlist, dbidx, 0);
    }

    songlistutilCreateFromList (pcupd->musicdb,
        pcupd->itemlist [idx].plname, dbidxlist);
    nlistFree (dbidxlist);
  }
}
