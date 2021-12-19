#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#if _hdr_libintl
# include <libintl.h>
#endif

#include "musicdb.h"
#include "tagdef.h"
#include "bdjstring.h"

int
main (int argc, char *argv[])
{
  // darwin: add /opt/local/bin to the path
  // windows: make sure the ./bin directory is in the path (for .dlls)
  // darwin: set DYLD_FALLBACK_LIBRARY_PATH to:
  //      /Applications/VLC.app/Contents/MacOS/lib/

  setlocale (LC_ALL, "");
  bindtextdomain ("bdj", "locale");
  textdomain ("bdj");

  tagdefInit ();
  dbOpen ("data/musicdb.dat");
  return 0;
}
