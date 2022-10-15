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
#include "pathbld.h"
#include "pathutil.h"
#include "song.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"

orgopt_t *
orgoptAlloc (void)
{
  orgopt_t      *orgopt;
  slist_t       *dflist;
  slistidx_t    dfiteridx;
  slist_t       *list;
  char          *value;
  char          *p;
  char          dispstr [MAXPATHLEN];
  char          path [MAXPATHLEN];

  pathbldMakePath (path, sizeof (path),
      "orgopt", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);

  tagdefInit ();

  if (! fileopFileExists (path)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: org: missing %s", path);
    return NULL;
  }

  orgopt = malloc (sizeof (orgopt_t));
  assert (orgopt != NULL);

  orgopt->df = datafileAllocParse ("org", DFTYPE_LIST, path, NULL, 0);
  dflist = datafileGetList (orgopt->df);

  list = slistAlloc ("org-disp", LIST_UNORDERED, free);

  slistStartIterator (dflist, &dfiteridx);
  while ((value = slistIterateKey (dflist, &dfiteridx)) != NULL) {
    org_t       *org;
    slistidx_t  piteridx;
    int         orgkey;
    int         tagkey;

    org = orgAlloc (value);

    dispstr [0] = '\0';
    orgStartIterator (org, &piteridx);
    while ((orgkey = orgIterateOrgKey (org, &piteridx)) >= 0) {
      if (orgkey == ORG_TEXT) {
        /* leading or trailing characters */
        p = orgGetText (org, piteridx);
        strlcat (dispstr, p, sizeof (dispstr));
      } else {
        tagkey = orgGetTagKey (orgkey);
        strlcat (dispstr, tagdefs [tagkey].displayname, sizeof (dispstr));
        if (orgkey == ORG_TRACKNUM0) {
          strlcat (dispstr, "0", sizeof (dispstr));
        }
      }
    }

    if (isWindows ()) {
      pathWinPath (dispstr, sizeof (dispstr));
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
    datafileFree (orgopt->df);
    slistFree (orgopt->orgList);
    free (orgopt);
  }
}

slist_t *
orgoptGetList (orgopt_t *orgopt)
{
  if (orgopt == NULL) {
    return NULL;
  }
  return orgopt->orgList;
}
