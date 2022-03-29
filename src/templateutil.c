#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"
#include "templateutil.h"

static void templateCopy (const char *from, const char *to, const char *color);

void
templateImageCopy (const char *color)
{
  char        localesfx [20];
  char        tbuff [MAXPATHLEN];
  char        from [MAXPATHLEN];
  char        to [MAXPATHLEN];
  slist_t     *dirlist;
  slistidx_t  iteridx;
  char        *fname;

  snprintf (localesfx, sizeof (localesfx), ".%s", sysvarsGetStr (SV_LOCALE_SHORT));

  pathbldMakePath (tbuff, sizeof (tbuff), "",
      "", "", PATHBLD_MP_TEMPLATEDIR);

  dirlist = filemanipBasicDirList (tbuff, ".svg");
  slistStartIterator (dirlist, &iteridx);
  while ((fname = slistIterateKey (dirlist, &iteridx)) != NULL) {
    pathbldMakePath (from, sizeof (from), "",
        fname, "", PATHBLD_MP_TEMPLATEDIR);
    pathbldMakePath (to, sizeof (to), "",
        fname, "", PATHBLD_MP_IMGDIR);

    templateCopy (from, to, color);
  }
  slistFree (dirlist);
}

/* internal routines */

static void
templateCopy (const char *from, const char *to, const char *color)
{
  char      localesfx [20];
  char      tbuff [MAXPATHLEN];

  if (! fileopFileExists (from)) {
    return;
  }

  snprintf (localesfx, sizeof (localesfx), ".%s", sysvarsGetStr (SV_LOCALE));
  strlcpy (tbuff, from, MAXPATHLEN);
  strlcat (tbuff, localesfx, MAXPATHLEN);
  if (fileopFileExists (tbuff)) {
    from = tbuff;
  } else {
    snprintf (localesfx, sizeof (localesfx), ".%s", sysvarsGetStr (SV_LOCALE_SHORT));
    strlcpy (tbuff, from, MAXPATHLEN);
    strlcat (tbuff, localesfx, MAXPATHLEN);
    if (fileopFileExists (tbuff)) {
      from = tbuff;
    }
  }

  if (strcmp (color, "#ffa600") == 0) {
    filemanipCopy (from, to);
  } else {
    char    *data;
    char    *p;
    FILE    *fh;
    size_t  len;

    data = filedataReadAll (from, &len);
    fh = fopen (to, "w");
    p = strstr (data, "#ffa600");
    while (p != NULL) {
      memcpy (p, color, 7);
      ++p;
      p = strstr (p, "#ffa600");
    }
    fwrite (data, len, 1, fh);
    fclose (fh);
    free (data);
  }
}
