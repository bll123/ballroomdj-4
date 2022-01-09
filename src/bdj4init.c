#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
  mstime_t   mt;
  mstime_t   dbmt;
  int       c = 0;
  int       option_index = 0;
  char      tbuff [MAXPATHLEN];
  int       loglevel = LOG_IMPORTANT | LOG_MAIN;

  static struct option bdj_options [] = {
    { "main",       no_argument,        NULL,   0 },
    { "profile",    required_argument,  NULL,   'p' },
    { "debug",      required_argument,  NULL,   'd' },
    { NULL,         0,                  NULL,   0 }
  };

  mstimestart (&mt);
  initLocale ();
  sysvarsInit (argv[0]);

  datautilMakePath (tbuff, sizeof (tbuff), "", sysvars [SV_HOSTNAME], "", DATAUTIL_MP_NONE);
  if (! fileopExists (tbuff)) {
    fileopMakeDir (tbuff);
  }

  while ((c = getopt_long (argc, argv, "", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        if (optarg) {
          loglevel = atol (optarg);
        }
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

  if (chdir (sysvars [SV_BDJ4DIR]) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvars [SV_BDJ4DIR]);
    exit (1);
  }
  bdjvarsInit ();

  logStart ("m", loglevel);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  bdjvarsdfloadInit ();
  bdjoptInit ();

  bdjoptSetLong (OPT_G_DEBUGLVL, loglevel);

  mstimestart (&dbmt);
  datautilMakePath (tbuff, MAXPATHLEN, "",
      MUSICDB_FNAME, MUSICDB_EXT, DATAUTIL_MP_NONE);
  dbOpen (tbuff);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Database read: %ld items in %ld ms", dbCount(), mstimeend (&dbmt));
  logMsg (LOG_SESS, LOG_IMPORTANT, "Total init time: %ld ms", mstimeend (&mt));

  return 0;
}

void
bdj4shutdown (void)
{
  mstime_t       mt;

  mstimestart (&mt);
  bdjoptFree ();
  dbClose ();
  bdjvarsdfloadCleanup ();
  bdjvarsCleanup ();
  logMsg (LOG_SESS, LOG_IMPORTANT, "init cleanup time: %ld ms", mstimeend (&mt));
  logEnd ();
}
