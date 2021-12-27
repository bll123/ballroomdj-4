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
#if _hdr_pthread
# include <pthread.h>
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
#include "songlist.h"

static void *openDatabase (void *);
static void initLocale (void);

#if _lib_pthread_create
static pthread_t   thread;
#endif

static void *
openDatabase (void *id)
{
  dbOpen ("data/musicdb.dat");
#if _lib_pthread_create
  pthread_exit (NULL);
#endif
  return NULL;
}

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
  int       c = 0;
  int       option_index = 0;
  static struct option bdj_options [] = {
    { "profile",    required_argument,  NULL,   'p' },
    { NULL,         0,                  NULL,   0 }
  };

  initLocale ();
  sysvarsInit ();

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

  logStart ();

  tagdefInit ();
#if _lib_pthread_create
  /* put the database read into a separate thread */
  pthread_create (&thread, NULL, openDatabase, NULL);
#else
  int junk;
  openDatabase (&junk);
#endif

  return 0;
}

void
bdj4finalizeStartup (void)
{
#if _lib_pthread_create
  pthread_join (thread, NULL);
#endif
}

void
bdj4shutdown (void)
{
  dbClose ();
  tagdefCleanup ();
  songlistCleanup ();
  logEnd ();
}
