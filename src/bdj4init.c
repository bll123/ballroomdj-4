#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#if _hdr_libintl
# include <libintl.h>
#endif
#if _hdr_unistd
# include <unistd.h>
#endif

#include "bdj4init.h"
#include "musicdb.h"
#include "tagdef.h"
#include "bdjstring.h"
#include "sysvars.h"
#include "log.h"
#include "tmutil.h"
#include "songlist.h"

#if 1 // temporary
# include "sortopt.h"
# include "sequence.h"
# include "dnctypes.h"
# include "dncspeeds.h"
# include "genre.h"
# include "rating.h"
# include "level.h"
# include "songlist.h"
#endif

static void initLocale (void);

static void
initLocale (void)
{
  setlocale (LC_ALL, "");
  bindtextdomain ("bdj", "locale");
  textdomain ("bdj");
  return;
}

int
bdj4startup (int argc, char *argv[])
{
  mtime_t   mt;
  mtime_t   dbmt;
  int       c = 0;
  int       option_index = 0;
  static struct option bdj_options [] = {
    { "profile",    required_argument,  NULL,   'p' },
    { "dbgstderr",  no_argument,        NULL,   's' },
    { NULL,         0,                  NULL,   0 }
  };

  mtimestart (&mt);
  initLocale ();
  sysvarsInit ();

  while ((c = getopt_long (argc, argv, "", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 's': {
        logStderr ();
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarSetLong (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  logStart ();

  tagdefInit ();
  mtimestart (&dbmt);
  dbOpen ("data/musicdb.dat");
  logVarMsg (LOG_SESS, "bdj4startup:",
      "Database read: %ld items in %ld ms", dbCount(), mtimeend (&dbmt));
  logVarMsg (LOG_SESS, "bdj4startup:",
      "Total startup time: %ld ms", mtimeend (&mt));

#if 1 // temporary
  datafile_t *so = sortoptAlloc ("data/sortopt.txt");
  sortoptFree (so);
  datafile_t *seq = sequenceAlloc ("data/standardrounds.seq");
  sequenceFree (seq);
  datafile_t *spd = dncspeedsAlloc ("data/dancespeeds.txt");
  dncspeedsFree (spd);
  datafile_t *typ = dnctypesAlloc ("data/dancetypes.txt");
  dnctypesFree (typ);
  datafile_t *r = ratingAlloc ("data/ratings.txt");
  ratingFree (r);
  datafile_t *g = genreAlloc ("data/genres.txt");
  genreFree (g);
  datafile_t *lvl = levelAlloc ("data/levels.txt");
  levelFree (lvl);
  datafile_t *sl = songlistAlloc ("data/dj.bll.01.songlist");
  songlistFree (sl);
#endif

  return 0;
}

void
bdj4shutdown (void)
{
  dbClose ();
  tagdefCleanup ();
  logEnd ();
}
