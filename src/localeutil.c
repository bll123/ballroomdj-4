#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <locale.h>
#if _hdr_libintl
# include <libintl.h>
#endif

#include "bdj4.h"
#include "localeutil.h"
#include "pathbld.h"

void
localeInit (void)
{
  char    tpath [MAXPATHLEN];

  setlocale (LC_ALL, "");
  pathbldMakePath (tpath, sizeof (tpath), "", "", "", PATHBLD_MP_LOCALEDIR);
  bindtextdomain ("bdj4", tpath);
#if _lib_bind_textdomain_codeset
  bind_textdomain_codeset ("bdj4", "UTF-8");
#endif
  textdomain ("bdj4");
  return;
}
