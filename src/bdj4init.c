#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
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
#include "process.h"
#include "fileutil.h"

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

static void
startPlayer (void)
{
  char      tbuff [MAXPATHLEN];
  pid_t     pid;

  logProcBegin ("startPlayer");
  strlcpy (tbuff, "bin/bdj4player", MAXPATHLEN);
  if (isWindows()) {
    strlcat (tbuff, ".exe", MAXPATHLEN);
  }
  int rc = processStart (tbuff, &pid);
  if (rc < 0) {
    logMsg (LOG_DBG, "player %s failed to start", tbuff);
  } else {
    logMsg (LOG_DBG, "player started pid: %zd", (ssize_t) pid);
  }
  logProcEnd ("startPlayer", "");
}

int
bdj4startup (int argc, char *argv[])
{
  mtime_t   mt;
  mtime_t   dbmt;
  int       c = 0;
  int       option_index = 0;
  char      tbuff [MAXPATHLEN];

  static struct option bdj_options [] = {
    { "profile",    required_argument,  NULL,   'p' },
    { "dbgstderr",  no_argument,        NULL,   's' },
    { NULL,         0,                  NULL,   0 }
  };

  mtimestart (&mt);
  initLocale ();
  sysvarsInit ();
  snprintf (tbuff, MAXPATHLEN, "data/%s", sysvars [SV_HOSTNAME]);
  if (! fileExists (tbuff)) {
    fileMakeDir (tbuff);
  }

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

  logStart ("m");

  tagdefInit ();
  mtimestart (&dbmt);
  dbOpen ("data/musicdb.dat");
  logMsg (LOG_SESS, "Database read: %ld items in %ld ms", dbCount(), mtimeend (&dbmt));
  logMsg (LOG_SESS, "Total startup time: %ld ms", mtimeend (&mt));

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

#if 1 // temporary
  startPlayer ();
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
