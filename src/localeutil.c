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

  pathbldMakePath (tbuff, sizeof (tbuff), "", "", "", PATHBLD_MP_LOCALEDIR);
  bindtextdomain ("bdj4", tbuff);
#if _lib_bind_textdomain_codeset
  bind_textdomain_codeset ("bdj4", "UTF-8");
#endif
  textdomain ("bdj4");

  strlcpy (lbuff, setlocale (LC_CTYPE, NULL), sizeof (lbuff));

  if (isWindows ()) {
    datafile_t  *df;
    slist_t     *list;
    char        *val;

    /* windows has non-standard names; convert them */
    pathbldMakePath (tbuff, sizeof (tbuff), "",
        "locale-win", ".txt", PATHBLD_MP_LOCALEDIR);
    df = datafileAllocParse ("locale-win", DFTYPE_KEY_VAL, tbuff,
        NULL, 0, DATAFILE_NO_LOOKUP);
    list = datafileGetList (df);
    val = slistGetData (list, lbuff);
    if (val != NULL) {
      strlcpy (lbuff, val, sizeof (lbuff));
    }
    datafileFree (df);
  }

  /* the sysvars variables must be reset */

  snprintf (tbuff, sizeof (tbuff), "%-.5s", lbuff);
  sysvarsSetStr (SV_LOCALE, tbuff);

  snprintf (tbuff, sizeof (tbuff), "%-.2s", lbuff);
  sysvarsSetStr (SV_SHORT_LOCALE, tbuff);

  return;
}
