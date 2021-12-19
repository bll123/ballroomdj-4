#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#if _hdr_libintl
# include <libintl.h>
#endif
#if _hdr_pthread
# include <pthread.h>
#endif

#include "musicdb.h"
#include "tagdef.h"
#include "bdjstring.h"
#include "sysvars.h"

void *openDatabase (void *);

void *
openDatabase (void *id)
{
  dbOpen ("data/musicdb.dat");
  pthread_exit (NULL);
}

int
main (int argc, char *argv[])
{
#if _lib_pthread_create
  pthread_t   thread;
#endif

  // darwin: add /opt/local/bin to the path
  // windows: make sure the ./bin directory is in the path (for .dlls)
  // darwin: set DYLD_FALLBACK_LIBRARY_PATH to:
  //      /Applications/VLC.app/Contents/MacOS/lib/

  setlocale (LC_ALL, "");
  bindtextdomain ("bdj", "locale");
  textdomain ("bdj");

  tagdefInit ();
#if _lib_pthread_create
  /* put the database read into a separate thread */
  pthread_create (&thread, NULL, openDatabase, NULL);
#else
  int junk;
  openDatabase (&junk);
#endif

  sysvarsInit ();

#if _lib_pthread_join
  pthread_join (thread, NULL);
#endif
printf ("count: %d\n", dbCount ());
  return 0;
}
