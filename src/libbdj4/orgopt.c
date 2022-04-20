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
#include "orgopt.h"
#include "orgutil.h"
#include "song.h"
#include "slist.h"
#include "tagdef.h"

orgopt_t *
orgoptAlloc (char *fname)
{
  orgopt_t      *orgopt;
  slist_t       *dflist;
  slistidx_t    dfiteridx;
  slist_t       *list;
  char          *value;
  char          *p;
  char          dispstr [MAXPATHLEN];

  tagdefInit ();

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
    org_t       *org;
    slist_t     *parsedlist;
    slistidx_t  piteridx;

    org = orgAlloc (value);
    parsedlist = orgGetList (org);

    dispstr [0] = '\0';
    slistStartIterator (parsedlist, &piteridx);
    while ((p = slistIterateKey (parsedlist, &piteridx)) != NULL) {
      int tagidx;

      tagidx = tagdefLookup (p);
      if (tagidx >= 0) {
        if (tagdefs [tagidx].isOrgTag) {
          strlcat (dispstr, tagdefs [tagidx].displayname, sizeof (dispstr));
        }
      } else if (strcmp (p, "TRACKNUMBER0") == 0) {
        strlcat (dispstr, "0", sizeof (dispstr));
        strlcat (dispstr, tagdefs [TAG_TRACKNUMBER].displayname, sizeof (dispstr));
      } else {
        /* leading or trailing characters */
        strlcat (dispstr, p, sizeof (dispstr));
      }
    }

    slistSetStr (list, dispstr, value);
    orgFree (org);
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
    free (orgopt);
  }
}

slist_t *
orgoptGetList (orgopt_t *orgopt)
{
  return orgopt->orgList;
}
