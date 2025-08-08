/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "slist.h"
#include "sortopt.h"
#include "tagdef.h"

typedef struct sortopt {
  datafile_t      *df;
  slist_t         *sortoptList;
} sortopt_t;

NODISCARD
sortopt_t *
sortoptAlloc (void)
{
  sortopt_t     *sortopt;
  slist_t       *dflist;
  slistidx_t    dfiteridx;
  slist_t       *list;
  const char    *value;
  char          *tvalue;
  char          *p;
  char          *tokstr;
  char          dispstr [MAXPATHLEN];
  char          fname [MAXPATHLEN];
  char          *dp;
  char          *dend = dispstr + sizeof (dispstr);

  tagdefInit ();

  pathbldMakePath (fname, sizeof (fname), SORTOPT_FN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: sortopt: missing %s", fname);
    return NULL;
  }

  sortopt = mdmalloc (sizeof (sortopt_t));

  sortopt->df = datafileAllocParse ("sortopt", DFTYPE_LIST, fname, NULL, 0, DF_NO_OFFSET, NULL);
  dflist = datafileGetList (sortopt->df);

  list = slistAlloc ("sortopt-disp", LIST_UNORDERED, NULL);
  slistStartIterator (dflist, &dfiteridx);
  while ((value = slistIterateKey (dflist, &dfiteridx)) != NULL) {
    tvalue = mdstrdup (value);

    dispstr [0] = '\0';
    dp = dispstr;
    p = strtok_r (tvalue, " ", &tokstr);
    while (p != NULL) {
      int tagidx;

      if (*dispstr) {
        dp = stpecpy (dp, dend, " / ");
      }

      tagidx = tagdefLookup (p);
      if (tagidx >= 0 && tagidx < TAG_KEY_MAX) {
        dp = stpecpy (dp, dend, tagdefs [tagidx].displayname);
      }

      p = strtok_r (NULL, " ", &tokstr);
    }

    slistSetStr (list, dispstr, value);
    mdfree (tvalue);
  }
  slistSort (list);
  sortopt->sortoptList = list;

  return sortopt;
}

void
sortoptFree (sortopt_t *sortopt)
{
  if (sortopt != NULL) {
    datafileFree (sortopt->df);
    slistFree (sortopt->sortoptList);
    mdfree (sortopt);
  }
}

slist_t *
sortoptGetList (sortopt_t *sortopt)
{
  if (sortopt == NULL) {
    return NULL;
  }

  return sortopt->sortoptList;
}
