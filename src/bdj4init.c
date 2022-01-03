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
#include "bdjvars.h"
#include "bdjvarsdfload.h"
#include "log.h"
#include "tmutil.h"
#include "fileop.h"
#include "portability.h"
#include "datautil.h"
#include "bdjopt.h"

#if 1 // temporary
# include "sequence.h"
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
  char      tbuff [MAXPATHLEN];

  static struct option bdj_options [] = {
    { "profile",    required_argument,  NULL,   'p' },
    { NULL,         0,                  NULL,   0 }
  };

  mtimestart (&mt);
  initLocale ();
  sysvarsInit (argv[0]);

  snprintf (tbuff, MAXPATHLEN, "data/%s", sysvars [SV_HOSTNAME]);
  if (! fileopExists (tbuff)) {
    fileopMakeDir (tbuff);
  }

  while ((c = getopt_long (argc, argv, "", bdj_options, &option_index)) != -1) {
    switch (c) {
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

  if (chdir (sysvars [SV_BDJ4DIR]) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvars [SV_BDJ4DIR]);
    exit (1);
  }
  bdjvarsInit ();

  logStart ("m", LOG_LVL_5);
  logMsg (LOG_SESS, LOG_LVL_1, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  bdjvarsdfloadInit ();
  bdjoptInit ();

  mtimestart (&dbmt);
  datautilMakePath (tbuff, MAXPATHLEN, "",
      MUSICDB_FNAME, MUSICDB_EXT, DATAUTIL_MP_NONE);
  dbOpen (tbuff);
  logMsg (LOG_SESS, LOG_LVL_1, "Database read: %ld items in %ld ms", dbCount(), mtimeend (&dbmt));
  logMsg (LOG_SESS, LOG_LVL_1, "Total startup time: %ld ms", mtimeend (&mt));

#if 1 // temporary
  datafile_t *seq = sequenceAlloc ("data/standardrounds.seq");
  sequenceFree (seq);
  datafile_t *sl = songlistAlloc ("data/dj.bll.01.songlist");
  songlistFree (sl);
#endif

  return 0;
}

void
bdj4shutdown (void)
{
  bdjoptFree ();
  dbClose ();
  bdjvarsdfloadCleanup ();
  bdjvarsCleanup ();
  logEnd ();
}
