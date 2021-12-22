#include "bdjconfig.h"

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

#include "musicdb.h"
#include "tagdef.h"
#include "bdjstring.h"
#include "sysvars.h"
#include "utility.h"
#include "log.h"

static void *openDatabase (void *);
static void initLocale (void);

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
main (int argc, char *argv[])
{
#if _lib_pthread_create
  pthread_t   thread;
#endif

  initLocale ();
  logStart ();
  sysvarsInit ();

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

#if _lib_pthread_join
  pthread_join (thread, NULL);
#endif
  logEnd ();
  return 0;
}
