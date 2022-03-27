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
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "orgutil.h"
#include "slist.h"

orgopt_t *
orgoptAlloc (char *fname)
{
  orgopt_t      *orgopt;
  slist_t       *dflist;
  slistidx_t    dfiteridx;
  slist_t       *list;
  char          *value;
  char          *tvalue;
  char          *p;
  char          *tokstr;
  char          *tokstrB;
  char          dispstr [MAXPATHLEN];

  if (! fileopFileExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: org: missing %s", fname);
    return NULL;
  }

  orgopt = malloc (sizeof (orgopt_t));
  assert (orgopt != NULL);

  orgopt->df = datafileAllocParse ("org", DFTYPE_LIST, fname, NULL, 0,
      DATAFILE_NO_LOOKUP);
  dflist = datafileGetList (orgopt->df);

  list = slistAlloc ("org-disp", LIST_UNORDERED, free);
  slistStartIterator (dflist, &dfiteridx);
  while ((value = slistIterateKey (dflist, &dfiteridx)) != NULL) {
    tvalue = strdup (value);

    dispstr [0] = '\0';
    p = strtok_r (tvalue, "{}", &tokstr);

    while (p != NULL) {
      p = strtok_r (p, "%", &tokstrB);
      while (p != NULL) {
        if (strcmp (p, "ALBUM") == 0) {
          strlcat (dispstr, _("Album"), sizeof (dispstr));
        } else if (strcmp (p, "ALBUMARTIST") == 0) {
          strlcat (dispstr, _("Album Artist"), sizeof (dispstr));
        } else if (strcmp (p, "ARTIST") == 0) {
          strlcat (dispstr, _("Artist"), sizeof (dispstr));
        } else if (strcmp (p, "COMPOSER") == 0) {
          strlcat (dispstr, _("Composer"), sizeof (dispstr));
        } else if (strcmp (p, "DANCE") == 0) {
          strlcat (dispstr, _("Dance"), sizeof (dispstr));
        } else if (strcmp (p, "DISC") == 0) {
          strlcat (dispstr, _("Disc"), sizeof (dispstr));
        } else if (strcmp (p, "GENRE") == 0) {
          strlcat (dispstr, _("Genre"), sizeof (dispstr));
        } else if (strcmp (p, "TITLE") == 0) {
          strlcat (dispstr, _("Title"), sizeof (dispstr));
        } else if (strcmp (p, "TRACKNUM") == 0) {
          strlcat (dispstr, _("Track Number"), sizeof (dispstr));
        } else if (strcmp (p, "TRACKNUM0") == 0) {
          strlcat (dispstr, "0", sizeof (dispstr));
          strlcat (dispstr, _("Track Number"), sizeof (dispstr));
        } else {
          /* leading or trailing characters */
          strlcat (dispstr, p, sizeof (dispstr));
        }
        p = strtok_r (NULL, "%", &tokstrB);
      }

      p = strtok_r (NULL, "{}", &tokstr);
    }

    slistSetStr (list, dispstr, value);
    free (tvalue);
  }
  slistSort (list);
  orgopt->orgList = list;

  return orgopt;
}

void
orgoptFree (orgopt_t *orgopt)
{
  if (orgopt != NULL) {
    if (orgopt->df != NULL) {
      datafileFree (orgopt->df);
    }
    if (orgopt->orgList != NULL) {
      slistFree (orgopt->orgList);
    }
  }
}

slist_t *
orgoptGetList (orgopt_t *orgopt)
{
  return orgopt->orgList;
}
