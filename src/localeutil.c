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
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"

void
localeInit (void)
{
  char        lbuff [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  char        tmp [MAXPATHLEN];
  bool        useutf8ext = false;

  /* on non-windows, the locale will already be set correctly in sysvars */
  /* if SV_LOCALE_SET is true, the locale was loaded from the */
  /* data/locale.txt file, and there is no need to do the windows processing */
  strlcpy (lbuff, sysvarsGetStr (SV_LOCALE), sizeof (lbuff));

  if (isWindows () && sysvarsGetNum (SVL_LOCALE_SET) == 0) {
    datafile_t  *df;
    slist_t     *list;
    char        *val;

    strlcpy (lbuff, setlocale (LOC_LC_MESSAGES, NULL), sizeof (lbuff));

    /* windows has non-standard names; convert them */
    pathbldMakePath (tbuff, sizeof (tbuff), "",
        "locale-win", ".txt", PATHBLD_MP_LOCALEDIR);
    df = datafileAllocParse ("locale-win", DFTYPE_KEY_VAL, tbuff,
        NULL, 0, DATAFILE_NO_LOOKUP);
    list = datafileGetList (df);
    val = slistGetStr (list, lbuff);
    if (val != NULL) {
      strlcpy (lbuff, val, sizeof (lbuff));
    }
    datafileFree (df);

    /* the sysvars variables must be reset */

    snprintf (tbuff, sizeof (tbuff), "%-.5s", lbuff);
    sysvarsSetStr (SV_LOCALE, tbuff);
    snprintf (tbuff, sizeof (tbuff), "%-.2s", lbuff);
    sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
  }

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
    snprintf (tmp, sizeof (tmp), "LC_ALL=%s", tbuff);
    putenv (tmp);
  }

  pathbldMakePath (lbuff, sizeof (lbuff), "", "", "", PATHBLD_MP_LOCALEDIR);
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
