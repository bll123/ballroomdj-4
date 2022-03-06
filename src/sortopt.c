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
#include "slist.h"
#include "sortopt.h"

sortopt_t *
sortoptAlloc (char *fname)
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

  if (! fileopFileExists (fname)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: sortopt: missing %s", fname);
    return NULL;
  }

  sortopt = malloc (sizeof (sortopt_t));
  assert (sortopt != NULL);

  sortopt->df = datafileAllocParse ("sortopt", DFTYPE_LIST, fname, NULL, 0,
      DATAFILE_NO_LOOKUP);
  dflist = datafileGetList (sortopt->df);

  list = slistAlloc ("sortopt-disp", LIST_UNORDERED, free);
  slistStartIterator (dflist, &dfiteridx);
  while ((value = slistIterateKey (dflist, &dfiteridx)) != NULL) {
    tvalue = strdup (value);

    dispstr [0] = '\0';
    p = strtok_r (tvalue, " ", &tokstr);
    while (p != NULL) {
      if (*dispstr) {
        strlcat (dispstr, " / ", sizeof (dispstr));
      }
      if (strcmp (p, "ALBUM") == 0) {
        strlcat (dispstr, _("Album"), sizeof (dispstr));
      }
      if (strcmp (p, "ALBUMARTIST") == 0) {
        strlcat (dispstr, _("Album Artist"), sizeof (dispstr));
      }
      if (strcmp (p, "ARTIST") == 0) {
        strlcat (dispstr, _("Artist"), sizeof (dispstr));
      }
      if (strcmp (p, "DANCE") == 0) {
        strlcat (dispstr, _("Dance"), sizeof (dispstr));
      }
      if (strcmp (p, "DANCELEVEL") == 0) {
        strlcat (dispstr, _("Dance Level"), sizeof (dispstr));
      }
      if (strcmp (p, "DANCERATING") == 0) {
        strlcat (dispstr, _("Dance Rating"), sizeof (dispstr));
      }
      if (strcmp (p, "DATEADDED") == 0) {
        strlcat (dispstr, _("Date Added"), sizeof (dispstr));
      }
      if (strcmp (p, "GENRE") == 0) {
        strlcat (dispstr, _("Genre"), sizeof (dispstr));
      }
      if (strcmp (p, "TITLE") == 0) {
        strlcat (dispstr, _("Title"), sizeof (dispstr));
      }
      if (strcmp (p, "TRACK") == 0) {
        strlcat (dispstr, _("Track"), sizeof (dispstr));
      }
      if (strcmp (p, "UPDATETIME") == 0) {
        strlcat (dispstr, _("Last Updated"), sizeof (dispstr));
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
    if (sortopt->df != NULL) {
      datafileFree (sortopt->df);
    }
    if (sortopt->sortoptList != NULL) {
      slistFree (sortopt->sortoptList);
    }
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
