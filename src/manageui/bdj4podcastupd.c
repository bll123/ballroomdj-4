/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
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

static int podcastupdGetASKey (void);
static void podcastupdProcess (musicdb_t *musicdb, int askey);
static bool podcastupdProcessPodcast (musicdb_t *musicdb, const char *plname, int askey);

int
main (int argc, char *argv[])
{
  uint32_t    flags;
  musicdb_t   *musicdb = NULL;
  slist_t     *filelist;
  int         count;
  int         askey;

#if BDJ4_MEM_DEBUG
  mdebugInit ("podu");
#endif

  /* do the startup w/o the DB first */
  flags = BDJ4_INIT_NO_DB_LOAD;
  bdj4startup (argc, argv, NULL, "podu", ROUTE_NONE, &flags);
  logProcBegin ();

  filelist = playlistGetPlaylistNames (PL_LIST_PODCAST, NULL);
  count = slistGetCount (filelist);
  askey = podcastupdGetASKey ();
fprintf (stderr, "count: %d askey: %d\n", count, askey);
  bdj4shutdown (ROUTE_NONE, NULL);
  slistFree (filelist);

  if (count > 0 && askey >= 0) {
    /* the database now needs to be loaded */
    flags = BDJ4_INIT_ALL;
    bdj4startup (argc, argv, &musicdb, "podu", ROUTE_PODCAST_UPD, &flags);

    podcastupdProcess (musicdb, askey);

    bdj4shutdown (ROUTE_PODCAST_UPD, musicdb);
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
podcastupdProcess (musicdb_t *musicdb, int askey)
{
  slist_t     *filelist;
  slistidx_t  iteridx;
  const char  *nm;
  bool        newsongs = false;

  filelist = playlistGetPlaylistNames (PL_LIST_PODCAST, NULL);
  slistStartIterator (filelist, &iteridx);
  while ((nm = slistIterateKey (filelist, &iteridx)) != NULL) {
    newsongs |= podcastupdProcessPodcast (musicdb, nm, askey);
  }
  slistFree (filelist);
}

static bool
podcastupdProcessPodcast (musicdb_t *musicdb, const char *plname, int askey)
{
  podcast_t   *podcast;
  int         retain;
  imppl_t     *imppl;
  bool        newsongs;
  slist_t     *songidxlist;

  logMsg (LOG_DBG, LOG_INFO, "process %s", plname);

  podcast = podcastLoad (plname);
  if (podcast == NULL) {
    return false;
  }

  retain = podcastGetNum (podcast, PODCAST_RETAIN);

  songidxlist = slistAlloc ("podu-imppl-song-idx", LIST_UNORDERED, NULL);
  imppl = impplInit (songidxlist, musicdb, AUDIOSRC_TYPE_PODCAST,
      podcastGetStr (podcast, PODCAST_URI), plname, plname, askey);
  while (impplProcess (imppl) == false) {
    ;
  }
  impplFinalize (imppl);
  newsongs = impplHaveNewSongs (imppl);

  podcastFree (podcast);
  impplFree (imppl);
  slistFree (songidxlist);

  return newsongs;
}
