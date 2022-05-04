#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include <locale.h>
#if _hdr_libintl
# include <libintl.h>
#endif

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "datafile.h"
#include "localeutil.h"
#include "osutils.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"

void
localeInit (void)
{
  char        lbuff [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  bool        useutf8ext = false;


  /* get the locale from the environment */
  setlocale (LC_ALL, "");

  /* these will be incorrect for windows */

  sysvarsSetStr (SV_LOCALE_SYSTEM, osGetLocale (tbuff, sizeof (tbuff)));
  snprintf (tbuff, sizeof (tbuff), "%-.5s", sysvarsGetStr (SV_LOCALE_SYSTEM));
  /* windows uses en-US rather than en_US */
  tbuff [2] = '_';
  sysvarsSetStr (SV_LOCALE, tbuff);
  snprintf (tbuff, sizeof (tbuff), "%-.2s", sysvarsGetStr (SV_LOCALE_SYSTEM));
  sysvarsSetStr (SV_LOCALE_SHORT, tbuff);

  strlcpy (lbuff, sysvarsGetStr (SV_LOCALE), sizeof (lbuff));

  if (isWindows ()) {
    if (atof (sysvarsGetStr (SV_OSVERS)) >= 10.0) {
      if (atoi (sysvarsGetStr (SV_OSBUILD)) >= 1803) {
        useutf8ext = true;
      }
    }
  } else {
    useutf8ext = true;
  }

  if (useutf8ext) {
    snprintf (tbuff, sizeof (tbuff), "%s.UTF-8", lbuff);
  } else {
    strlcpy (tbuff, lbuff, sizeof (tbuff));
  }

  if (isWindows ()) {
    /* windows doesn't work without this */
    osSetEnv ("LC_ALL", tbuff);
  }

  pathbldMakePath (lbuff, sizeof (lbuff), "", "", PATHBLD_MP_LOCALEDIR);
  bindtextdomain (GETTEXT_DOMAIN, lbuff);
  textdomain (GETTEXT_DOMAIN);
#if _lib_bind_textdomain_codeset
  bind_textdomain_codeset (GETTEXT_DOMAIN, "UTF-8");
#endif

  if (setlocale (LOC_LC_MESSAGES, tbuff) == NULL) {
    fprintf (stderr, "set of locale failed; unknown locale %s\n", tbuff);
  }

  return;
}
