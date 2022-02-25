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
#include "bdjstring.h"
#include "localeutil.h"
#include "pathbld.h"
#include "sysvars.h"

void
localeInit (void)
{
  char    tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff), "", "", "", PATHBLD_MP_LOCALEDIR);
  bindtextdomain ("bdj4", tbuff);
#if _lib_bind_textdomain_codeset
  bind_textdomain_codeset ("bdj4", "UTF-8");
#endif
  textdomain ("bdj4");

  /* the sysvars variables must be reset */

  /* this will be incorrect on windows */
  // ### FIX
  snprintf (tbuff, sizeof (tbuff), "%-.5s", setlocale (LC_ALL, NULL));
  sysvarsSetStr (SV_LOCALE, tbuff);

  snprintf (tbuff, sizeof (tbuff), "%-.2s", setlocale (LC_ALL, NULL));
  stringToLower (tbuff);  // for windows
  sysvarsSetStr (SV_SHORT_LOCALE, tbuff);

  return;
}
