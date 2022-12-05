/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "pathbld.h"
#include "slist.h"
#include "sortopt.h"
#include "tagdef.h"

typedef struct sortopt {
  datafile_t      *df;
  slist_t         *sortoptList;
} sortopt_t;

sortopt_t *
sortoptAlloc (void)
{
  sortopt_t     *sortopt;
  slist_t       *dflist;
  slistidx_t    dfiteridx;
  slist_t       *list;
  char          *value;
  char          *tvalue;
  char          *p;
  char          *tokstr;
  char          dispstr [MAXPATHLEN];
  char          fname [MAXPATHLEN];

  tagdefInit ();

  pathbldMakePath (fname, sizeof (fname), "sortopt",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: sortopt: missing %s", fname);
    return NULL;
  }

  sortopt = malloc (sizeof (sortopt_t));
  assert (sortopt != NULL);

  sortopt->df = datafileAllocParse ("sortopt", DFTYPE_LIST, fname, NULL, 0);
  dflist = datafileGetList (sortopt->df);

  list = slistAlloc ("sortopt-disp", LIST_UNORDERED, free);
  slistStartIterator (dflist, &dfiteridx);
  while ((value = slistIterateKey (dflist, &dfiteridx)) != NULL) {
    tvalue = strdup (value);

    dispstr [0] = '\0';
    p = strtok_r (tvalue, " ", &tokstr);
    while (p != NULL) {
      int tagidx;

      if (*dispstr) {
        strlcat (dispstr, " / ", sizeof (dispstr));
      }

      tagidx = tagdefLookup (p);
      if (tagidx >= 0) {
        strlcat (dispstr, tagdefs [tagidx].displayname, sizeof (dispstr));
      }

      p = strtok_r (NULL, " ", &tokstr);
    }

    slistSetStr (list, dispstr, value);
    free (tvalue);
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
    free (sortopt);
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
