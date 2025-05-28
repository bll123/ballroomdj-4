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

#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "log.h"
#include "mdebug.h"
#include "playlist.h"
#include "podcast.h"
#include "podcastutil.h"
#include "slist.h"
#include "songlist.h"

static void podcastupdProcess (void);
static void podcastupdProcessPodcast (const char *plname);

int
main (int argc, char *argv[])
{
  uint32_t        flags;

#if BDJ4_MEM_DEBUG
  mdebugInit ("podu");
#endif

  flags = BDJ4_INIT_ALL;
  bdj4startup (argc, argv, NULL, "podu", ROUTE_NONE, &flags);
  logProcBegin ();

  podcastupdProcess ();

  logProcEnd ("");
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

static void
podcastupdProcess (void)
{
  slist_t     *filelist;
  slistidx_t  iteridx;
  const char  *nm;

  filelist = playlistGetPlaylistNames (PL_LIST_PODCAST, NULL);
  slistStartIterator (filelist, &iteridx);
  while ((nm = slistIterateKey (filelist, &iteridx)) != NULL) {
    podcastupdProcessPodcast (nm);
  }
}

static void
podcastupdProcessPodcast (const char *plname)
{
  podcast_t   *podcast;
  int         retain;

fprintf (stderr, "process: %s\n", plname);
  podcast = podcastLoad (plname);
  if (podcast == NULL) {
    return;
  }

  retain = podcastGetNum (podcast, PODCAST_RETAIN);
  podcastFree (podcast);
}
