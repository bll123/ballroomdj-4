#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <errno.h>
#if _hdr_libintl
# include <libintl.h>
#endif
#if _hdr_pthread
# include <pthread.h>
#endif
#if _hdr_bsd_string
# include <bsd/string.h>
#endif
#if _hdr_unistd
# include <unistd.h>
#endif

#include "bdj4main.h"
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
}

static void
initLocale (void)
{
  setlocale (LC_ALL, "");
  bindtextdomain ("bdj", "locale");
  textdomain ("bdj");
}

int
bdj4startup (int argc, char *argv[])
{
  initLocale ();
  sysvarsInit ();

  /* ### FIX later */
  if (argc > 1 && strcmp (argv[1], "-profile") == 0) {
    sysvarSetLong (SVL_BDJIDX, atol (argv[2]));
  }

  logStart ();

  /* may need to be moved to launcher */
  if (isMacOS()) {
    char  npath [MAXPATHLEN];

    char  *path = getenv ("PATH");
    strlcpy (npath, "PATH=", MAXPATHLEN);
    strlcat (npath, path, MAXPATHLEN);
    strlcat (npath, ":/opt/local/bin", MAXPATHLEN);
    putenv (npath);
    putenv("DYLD_FALLBACK_LIBRARY_PATH="
        "/Applications/VLC.app/Contents/MacOS/lib/");
  }

  /* may need to be moved to launcher */
  if (isWindows()) {
    char  npath [MAXPATHLEN];
    char  buff [MAXPATHLEN];
    char  cwd [MAXPATHLEN];

    char  *path = getenv ("PATH");
    strlcpy (npath, "PATH=", MAXPATHLEN);
    strlcat (npath, path, MAXPATHLEN);
    if (getcwd (cwd, MAXPATHLEN) == NULL) {
      logError (LOG_ERR, "main: getcwd", errno);
    }
    snprintf (buff, MAXPATHLEN, ";%s", cwd);
    strlcat (npath, buff, MAXPATHLEN);
    /* ### FIX need some file path handling routines */
    /* ### FIX the windows path probably needs to be updated
       within the launcher */
    snprintf (buff, MAXPATHLEN, ";%s\\..\\plocal\\lib", cwd);
    strlcat (npath, buff, MAXPATHLEN);
    putenv (npath);
  }

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
#if _lib_pthread_join
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
