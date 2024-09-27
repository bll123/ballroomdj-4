/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "orgopt.h"
#include "orgutil.h"
#include "pathbld.h"
#include "pathdisp.h"
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
  const char    *value;
  char          dispstr [MAXPATHLEN];
  char          path [MAXPATHLEN];
  char          *dp;
  char          *dend = dispstr + sizeof (dispstr);


  pathbldMakePath (path, sizeof (path),
      ORGOPT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);

  tagdefInit ();

  if (! fileopFileExists (path)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: org: missing %s", path);
    return NULL;
  }

  orgopt = mdmalloc (sizeof (orgopt_t));

  orgopt->df = datafileAllocParse ("org", DFTYPE_LIST, path, NULL, 0, DF_NO_OFFSET, NULL);
  dflist = datafileGetList (orgopt->df);

  list = slistAlloc ("org-disp", LIST_UNORDERED, NULL);

  slistStartIterator (dflist, &dfiteridx);
  while ((value = slistIterateKey (dflist, &dfiteridx)) != NULL) {
    org_t       *org;
    slistidx_t  piteridx;
    int         orgkey;
    int         tagkey;

    org = orgAlloc (value);

    dispstr [0] = '\0';
    dp = dispstr;
    orgStartIterator (org, &piteridx);
    while ((orgkey = orgIterateOrgKey (org, &piteridx)) >= 0) {
      if (orgkey == ORG_TEXT) {
        const char  *tmp;
        /* leading or trailing characters */
        tmp = orgGetText (org, piteridx);
        dp = stpecpy (dp, dend, tmp);
      } else {
        tagkey = orgGetTagKey (orgkey);
        dp = stpecpy (dp, dend, tagdefs [tagkey].displayname);
        if (orgkey == ORG_TRACKNUM0) {
          dp = stpecpy (dp, dend, "0");
        }
      }
    }

    pathDisplayPath (dispstr, sizeof (dispstr));
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
    mdfree (orgopt);
  }
}

/* for testing */
slist_t *
orgoptGetList (orgopt_t *orgopt)
{
  if (orgopt == NULL) {
    return NULL;
  }
  return orgopt->orgList;
}
